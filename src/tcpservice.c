
#include <stdlib.h>
#include <stdarg.h>

#include "console.h"
#include "tcpservice.h"
#include "cbuf.h"

#define TCPVERBERR 1

static void tcp_service_aborted (void* svc, err_t err)
{
	tcpservice_t* s = (tcpservice_t*)svc;
	s->tcp = NULL; // unallocated by caller
	if (s->cb_cleanup)
		s->cb_cleanup(s);
	else if (s->sendbuf)
		os_free(s->sendbuf);
	os_free(s);
}

static bool tcp_service_check_shutdown (tcpservice_t* s)
{
	if (s->is_closing && s->tcp && cbuf_is_empty(&s->send_buffer))
	{
		// everything is received by peer
		tcp_close(s->tcp);
		tcp_service_aborted(s, ERR_OK);
		return true;
	}
	return false;
}

void tcp_service_close (tcpservice_t* s)
{
	if (s->tcp && !s->is_closing)
	{
		s->is_closing = true;
		if (s->cb_closing)
			s->cb_closing(s);
	}
}

static void tcp_service_error (void* svc, err_t err)
{
#if TCPVERBERR
	const char* msg;
	switch (err)
	{
	case ERR_OK:		msg="ok"; break;
	case ERR_MEM:		msg="mem"; break;
	case ERR_BUF:		msg="buf"; break;
	case ERR_TIMEOUT:	msg="timeout"; break;
	case ERR_RTE:		msg="route"; break;
	case ERR_INPROGRESS:	msg="in progress"; break;
	case ERR_VAL:		msg="illegal value"; break;
	case ERR_WOULDBLOCK:	msg="would block"; break;
	case ERR_ABRT:		msg="aborted"; break;
	case ERR_RST:		msg="reset"; break;
	case ERR_CLSD:		msg="closed"; break;
	case ERR_CONN:		msg="not connected"; break;
	case ERR_ARG:		msg="illegal arg"; break;
	case ERR_USE:		msg="address in use"; break;
	case ERR_IF:		msg="intf error"; break;
	case ERR_ISCONN:	msg="already connected"; break;
#ifdef ERR_ALREADY
	case ERR_ALREADY:	msg="already connecting"; break;
#endif
	default:		msg="?";
	}
#else
	const char* msg = "?";
#endif	

	tcpservice_t* peer = (tcpservice_t*)svc;

	LOGSERIAL(ERR_IS_FATAL(err)? LOG_ERR: LOG_WARN, "TCP(%s): %serror %d (%s)",
		peer->name,
		ERR_IS_FATAL(err)? "fatal ": "",
		(int)err,
		msg);

	if (ERR_IS_FATAL(err))
		tcp_service_close(peer);
}

// trigger write-from-circular-buffer
static err_t cbuf_tcp_send (tcpservice_t* tcp)
{
	err_t err = ERR_OK;
	size_t sndbuf, sendsize;
	char* data;

	if (!tcp->tcp)
		return ERR_OK;

	while ((sndbuf = tcp_sndbuf(tcp->tcp)) > 0)
	{
		if ((sendsize = cbuf_read_ptr(&tcp->send_buffer, &data, sndbuf)) == 0)
			break;
		if ((err = tcp_write(tcp->tcp, data, sendsize, /*tcpflags=0=PUSH,NOCOPY*/0)) != ERR_OK)
		{
			tcp_service_error(tcp, err);
			break;
		}
	}

        if (err == ERR_OK && (err = tcp_output(tcp->tcp)) != ERR_OK)
		tcp_service_error(tcp, err);
	return err;
}

static void tcp_service_give_back (tcpservice_t* peer, pbuf_t* root_pbuf)
{
	size_t swallowed = 0;
	size_t pbuf_taken = 0;
	pbuf_t* pbuf = root_pbuf;
	while (pbuf)
	{
		// give data to user
		size_t acked_by_user = peer->cb_recv
			(
				peer,
				((char*)pbuf->payload) + pbuf_taken,
				pbuf->len - pbuf_taken
			);
		if (acked_by_user == 0)
			break;

		if ((pbuf_taken += acked_by_user) == pbuf->len)
		{
			// current pbuf is fully swallowed, next one
			pbuf = pbuf->next;
			pbuf_taken = 0;
		}
		swallowed += acked_by_user;
	}
	
	// report to lwip how much data
	// have been acknowledged by peer receive callback,
	// this increases receive window.
	// if user->cb_ack is defined, then it is responsible
	// to increase window by calling tcp_service_allow_more()
	if (swallowed && !peer->cb_ack)
		tcp_recved(peer->tcp, swallowed);
	if (swallowed != root_pbuf->tot_len)
		LOG(LOG_ERR, "%s: cb_recv() must comply, data lost", peer->name);
	pbuf_free(root_pbuf);
}

static err_t tcp_service_receive (void* svc, struct tcp_pcb* pcb, pbuf_t* pbuf, err_t err)
{
	tcpservice_t* peer = (tcpservice_t*)svc;
	if (err == ERR_OK)
	{
		if (pbuf)
			// feed user
			tcp_service_give_back(peer, pbuf);

		// send our output buffer
		return tcp_service_check_shutdown(peer)?
			ERR_CLSD:
			cbuf_tcp_send(peer);
	}

	// bad state
	tcp_service_error(peer, err);
	pbuf_free(pbuf);
	tcp_service_close(peer);
	return err;
}

static err_t tcp_service_ack (void *svc, struct tcp_pcb *pcb, u16_t len)
{
	tcpservice_t* peer = (tcpservice_t*)svc;

	// release hold data in send buffer
	cbuf_ack(&peer->send_buffer, len);
	// close now if requested and if all data are remotely received
	if (tcp_service_check_shutdown(peer))
		return ERR_CLSD;
	// user callback
	if (peer->cb_ack && len)
		peer->cb_ack(peer, len);
	// continue to send our data
	return cbuf_tcp_send(peer);
}

static err_t tcp_service_poll (void* svc, struct tcp_pcb* pcb)
{ 
	LWIP_UNUSED_ARG(pcb);
	tcpservice_t* peer = (tcpservice_t*)svc;
	if (peer->cb_poll)
		peer->cb_poll(peer);
	// trigger sending data
	return tcp_service_receive(peer, peer->tcp, NULL, ERR_OK);
}

static err_t tcp_service_incoming_peer (void* svc, struct tcp_pcb * peer_pcb, err_t err)
{
	LWIP_UNUSED_ARG(err);
	tcpservice_t* listener = (tcpservice_t*)svc;

	tcp_accepted(listener->tcp);
	tcpservice_t* peer = listener->cb_get_new_peer(listener);
	if (!peer)
		return ERR_MEM; //XXX handle this better

	peer->tcp = peer_pcb;
	peer->is_closing = 0;
	if (!peer->name)
		peer->name = listener->name;
	
	tcp_setprio(peer->tcp, TCP_PRIO_MIN); //XXX???
	tcp_recv(peer->tcp, tcp_service_receive);
	tcp_err(peer->tcp, tcp_service_aborted);
	tcp_poll(peer->tcp, tcp_service_poll, 4); //every two seconds of inactivity of the TCP connection
	tcp_sent(peer->tcp, tcp_service_ack);
	
	// peer/tcp_pcb association
	tcp_arg(peer->tcp, peer);
	
#if 0
	// disable nagle
	SERIAL_PRINTF("%s: nagle=%d\n", peer->name, !tcp_nagle_disabled(peer->tcp));
	tcp_nagle_disable(peer->tcp);
	SERIAL_PRINTF("%s: nagle=%d\n", peer->name, !tcp_nagle_disabled(peer->tcp));
#endif

	// start fighting
	return peer->cb_established? peer->cb_established(peer): ERR_OK;
}

int tcp_service_install (const char* name, tcpservice_t* s, int port)
{
	if (name)
		s->name = name;

	if (s->tcp)
	{
		console_printf("%s: server already started\n", name);
		return -1;
	}
	
	if (!s->cb_get_new_peer)
	{
		console_printf("%s: internal setup error, new-peer callback not set\n", name);
		return -1;
	}
	
	s->tcp = tcp_new();
	if (!s->tcp)
	{
		console_printf("%s: unable to allocate tcp\n", name);
		return -1;
	}
	
	tcp_bind(s->tcp, IP_ADDR_ANY, port);
	struct tcp_pcb* updated_tcp = tcp_listen(s->tcp);
	if (!updated_tcp)
	{
		os_free(s->tcp);
		s->tcp = NULL;
		console_printf("%s: Unable to listen (mem error)\n", name);
		return -1;
	}
	s->tcp = updated_tcp;

	tcp_accept(s->tcp, tcp_service_incoming_peer);
	tcp_arg(s->tcp, s);
	console_printf("%s: server accepting connections on port %d\n", name, port);
	return 0;
}

tcpservice_t* tcp_service_init_new_peer_sendbuf_size (char* sendbuf, size_t sendbufsize)
{
	tcpservice_t* peer = (tcpservice_t*)os_malloc(sizeof(tcpservice_t));
	if (!peer)
		return NULL;

	cbuf_init(&peer->send_buffer, peer->sendbuf = sendbuf, sendbufsize);
	
	peer->name = NULL;
	peer->cb_get_new_peer = NULL;
	peer->cb_established = NULL;
	peer->cb_closing = NULL;
	peer->cb_recv = NULL;
	peer->cb_ack = NULL;
	peer->cb_poll = NULL;
	peer->cb_cleanup = NULL;
	return peer;
}

tcpservice_t* tcp_service_init_new_peer_size (size_t sendbufsize)
{
	char* sendbuf;
	if (!sendbufsize)
		sendbuf = NULL;
	else if ((sendbuf = (char*)os_malloc(sendbufsize)) == NULL)
		return NULL;
	tcpservice_t* svc = tcp_service_init_new_peer_sendbuf_size(sendbuf, sendbufsize);
	if (!svc && sendbuf)
		os_free(sendbuf);
	return svc;
}
