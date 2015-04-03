
#include <stdlib.h>
#include <stdarg.h>

#include "console.h"
#include "tcpservice.h"
#include "cbuf.h"

#if 1 // 0 => debug tcpservice
#define D(x...)
#define TCPVERBERR 1 // 0:silent 1:fatal-only 2:all
#else
#define D(x...) SERIAL_PRINTF(x)
#define TCPVERBERR 2 // 0:silent 1:fatal-only 2:all
#endif

//XXX a configuration value for this?
//#define TCPVERBERR 2 // 0:silent 1:fatal-only 2:all

static bool tcp_service_check_shutdown (tcpservice_t* s)
{
	if (s->is_closing && s->tcp && cbuf_is_empty(&s->send_buffer) && s->pbuf == NULL)
	{
		// everything is sent and received pbuf are freed
		tcp_close(s->tcp);
		s->tcp = NULL;
		if (s->cb_cleanup)
			s->cb_cleanup(s);
		if (s->sendbuf)
			os_free(s->sendbuf);
		os_free(s);
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
	// verbose

	static const char* lwip_err_msg [] =
		{
			"OK", "MEM", "BUF", "TIMEOUT", "ROUTE", "INPROGRESS", "INVAL",
		#if TCPVERBERR > 1
			"WBLOCK", "ABORT", "RESET", "CLOSED", "INARG", "INUSE", "IFERR", "ISCONN"
		#endif // TCPVERBERR > 1
		};
	tcpservice_t* peer = (tcpservice_t*)svc;

	LOGSERIAL(ERR_IS_FATAL(err)? LOG_ERR: LOG_WARN, "TCP(%s): %serror %d (%s)",
		peer->name,
		ERR_IS_FATAL(err)? "fatal ": "",
		(int)err,
		err < 0 && -err < sizeof(lwip_err_msg) / sizeof(lwip_err_msg[0])? lwip_err_msg[-err]: "?");
#endif // TCPVERBERR > 0

	if (ERR_IS_FATAL(err))
		tcp_service_close(peer);
}

// tcp_send() is static: tcp_write() cannot be called by user's callbacks (it hangs lwip)
// trigger write-from-circular-buffer
static err_t cbuf_tcp_send (tcpservice_t* tcp)
{
	err_t err = ERR_OK;
	size_t sndbuf, sendsize;
	char* data;

D("1");
	if (!tcp->tcp)
		return ERR_OK;

	while ((sndbuf = tcp_sndbuf(tcp->tcp)) > 0)
	{
D("2(%d)", sndbuf);
		if ((sendsize = cbuf_read_ptr(&tcp->send_buffer, &data, sndbuf)) == 0)
			break;
D("3(%d)", sendsize);
		if ((err = tcp_write(tcp->tcp, data, sendsize, /*tcpflags=0=PUSH,NOCOPY*/0)) != ERR_OK)
		{
D("e(%d)",err);
			tcp_service_error(tcp, err);
			break;
		}
	}

        if (err == ERR_OK && (err = tcp_output(tcp->tcp)) != ERR_OK)
		tcp_service_error(tcp, err);
D("4");
	return err;
}

static void tcp_service_give_back (tcpservice_t* peer, pbuf_t* pbuf)
{
	if (pbuf)
	{
D("5");
		// a new pbuf has come
		if (peer->pbuf)
			pbuf_cat(peer->pbuf, pbuf);
		else
			peer->pbuf = pbuf;
	}

	size_t swallowed = 0;	
	while (peer->pbuf)
	{
D("6");
		// give data to user
		size_t acked_by_user = peer->cb_recv
			(
				peer,
				((char*)peer->pbuf->payload) + peer->pbuf_taken,
				peer->pbuf->len - peer->pbuf_taken
			);

		if (acked_by_user == 0)
			break;
D("7(%d)", acked_by_user);
		if ((peer->pbuf_taken += acked_by_user) == peer->pbuf->len)
		{
			// current pbuf is fully sallowed, skip+delete
			pbuf_t* deleteme = peer->pbuf;
			peer->pbuf = pbuf_dechain(peer->pbuf);
			peer->pbuf_taken = 0;
			pbuf_free(deleteme);
		}
		swallowed += acked_by_user;
	}
D("8(%d)",swallowed);
	// report to lwip how much data
	// have been acknowledged by peer receive callback
	if (swallowed)
		tcp_recved(peer->tcp, swallowed);
D("9");
}

static err_t tcp_service_receive (void* svc, struct tcp_pcb* pcb, pbuf_t* pbuf, err_t err)
{
	tcpservice_t* peer = (tcpservice_t*)svc;

	if (err == ERR_OK)
	{
D("\nr");pbuf_t* x=peer->pbuf;while(x){D("(%d)",x->len);x=x->next;}D("N");x=pbuf;while(x){D("(%d)",x->len);x=x->next;}
		// feed user with new and awaiting pbufs
		tcp_service_give_back(peer, pbuf);
D("b");
		// send our output buffer
		return cbuf_tcp_send(peer);
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
D("\nc(%d)", len);
	if (len)
		cbuf_ack(&peer->send_buffer, len);

	// feed user with awaiting pbufs 
	tcp_service_give_back(peer, NULL);
D("d");
	return tcp_service_check_shutdown(peer)?
		ERR_OK:
		cbuf_tcp_send(peer);
}

static err_t tcp_service_poll (void* svc, struct tcp_pcb* pcb)
{ 
	LWIP_UNUSED_ARG(pcb);
	tcpservice_t* peer = (tcpservice_t*)svc;
	return tcp_service_check_shutdown(peer)? ERR_OK: peer->cb_poll? peer->cb_poll(peer): ERR_OK;
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
	
	peer->pbuf = NULL;
	peer->pbuf_taken = 0;
	
	tcp_setprio(peer->tcp, TCP_PRIO_MIN); //XXX???
	tcp_recv(peer->tcp, tcp_service_receive);
	tcp_err(peer->tcp, tcp_service_error);
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

tcpservice_t* tcp_service_init_new_peer_sendbuf_sizelog2 (char* sendbuf, char sendbufsizelog2)
{
	tcpservice_t* peer = (tcpservice_t*)os_malloc(sizeof(tcpservice_t));
	if (!peer)
	{
		os_free(sendbuf);
		return NULL;
	}

	cbuf_init(&peer->send_buffer, peer->sendbuf = sendbuf, sendbufsizelog2);
	
	peer->name = NULL;
	peer->cb_get_new_peer = NULL;
	peer->cb_established = NULL;
	peer->cb_closing = NULL;
	peer->cb_recv = NULL;
	peer->cb_poll = NULL;
	peer->cb_cleanup = NULL;
	return peer;
}

tcpservice_t* tcp_service_init_new_peer_sizelog2 (char sendbufsizelog2)
{
	char* sendbuf;
	if (!sendbufsizelog2)
		sendbuf = NULL;
	else if ((sendbuf = (char*)os_malloc(1 << sendbufsizelog2)) == NULL)
		return NULL;
	return tcp_service_init_new_peer_sendbuf_sizelog2(sendbuf, sendbufsizelog2);
}
