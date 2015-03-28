
/// TODO virer await, utiliser pbuf->next?

#include <stdlib.h>
#include <stdarg.h>

#include "console.h"
#include "tcpservice.h"
#include "cbuf.h"

#define D(x...) SERIAL_PRINTF(x)

//XXX a configuration value for this?
#define TCPVERBERR 2 // 0:silent 1:fatal-only 2:all

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
	
	while ((sndbuf = tcp_sndbuf(tcp->tcp)) > 0)
	{
D("sndbuf=%d\n", sndbuf);
		if ((sendsize = cbuf_read_ptr(&tcp->send_buffer, &data, sndbuf)) == 0)
			break;
		if ((err = tcp_write(tcp->tcp, data, sendsize, /*tcpflags=0=PUSH,NOCOPY*/0)) != ERR_OK)
		{
D("x1\n");
			tcp_service_error(tcp, err);
			break;
		}
D("sent: %d\n", sendsize);
	}

        if ((err = tcp_output(tcp->tcp)) != ERR_OK)
{D("x2\n");
		tcp_service_error(tcp, err);
}
D("x22=%d\n",err);
	return err;
}

#if STOREPBUF

static void tcp_service_give_back (tcpservice_t* peer, pbuf_t* pbuf)
{

pbuf_t* pb = pbuf;
while (pb)
{
	D("recv %d -", pb->len);
	pb = pb->next;
}
D("\n");


	if (pbuf)
	{
D("x3\n");
		if (peer->pbuf)
		{
D("x4\n");
			// a new pbuf comes, we already have pbufs to process
			// store the new pbuf at the end of the current chain
			pbuf_t* it = peer->pbuf;
			while (it->next)
				it = it->next;
			it->next = pbuf;
		}
		else
			peer->pbuf = pbuf;
	}

	size_t swallowed = 0;	
	while (peer->pbuf)
	{
D("x5\n");
		// give data to user
		size_t acked_by_user = peer->cb_recv
			(
				peer,
				((char*)peer->pbuf->payload) + peer->pbuf_taken,
				peer->pbuf->len - peer->pbuf_taken
			);
D("f a=%d g=%d\n", acked_by_user, peer->pbuf->len - peer->pbuf_taken);
		if ((peer->pbuf_taken += acked_by_user) == peer->pbuf->len)
		{
D("x51\n");
			// pbuf fully sallowed, skip/delete
			pbuf_t* deleteme = peer->pbuf;
			peer->pbuf_taken = 0;
			peer->pbuf = peer->pbuf->next;
			pbuf_free(deleteme);
		}
		swallowed += acked_by_user;
	}
	// report to lwip how much data have been acknowledged
D("sw %d\n", swallowed);
	if (swallowed)
		tcp_recved(peer->tcp, swallowed);
}
#endif // STOREPBUF

static err_t tcp_service_receive (void* svc, struct tcp_pcb *pcb, struct pbuf *pbuf, err_t err)
{
	tcpservice_t* peer = (tcpservice_t*)svc;

	if (err == ERR_OK && pbuf != NULL)
	{
if (pbuf->next) SERIAL_PRINTF("CHAIN\n");
if (pbuf->len != pbuf->tot_len) SERIAL_PRINTF("CHAIN2\n");
		// feed user with awaiting pbufs, and this pbuf
#if STOREPBUF
		tcp_service_give_back(peer, pbuf);
#else		
		size_t all = 0;
		while (pbuf)
		{
			size_t acked = peer->cb_recv(peer, pbuf->payload, pbuf->len);
			if (acked != pbuf->len)
				SERIAL_PRINTF("OOOPS\n");
			all += pbuf->len;
			
			typeof(pbuf) delme = pbuf;
			pbuf = pbuf->next;
			pbuf_free(delme);
		}
		tcp_recved(peer->tcp, all);
#endif
		return cbuf_tcp_send(peer);
	}
	else
	{
D("x10\n");
		if (pbuf)
			pbuf_free(pbuf);
		tcp_service_close(peer);
		return ERR_OK;
	}
}

static bool tcp_service_check_shutdown (tcpservice_t* s)
{
	if (s->is_closing && cbuf_is_empty(&s->send_buffer))
	{
		tcp_close(s->tcp);
		s->tcp = NULL; // lwip will free this
		if (s->cb_cleanup)
			s->cb_cleanup(s);
		if (s->sendbuf)
			os_free(s->sendbuf);
		os_free(s);
#if STOREPBUF
		//XXX recursively pbuf_free s->pbuf
#endif
		return true;
	}
	return false;
}

static err_t tcp_service_ack (void *svc, struct tcp_pcb *pcb, u16_t len)
{
	tcpservice_t* peer = (tcpservice_t*)svc;
D("acked:%d\n", len);
	if (len)
		cbuf_ack(&peer->send_buffer, len);

#if 1
	// feed user with awaiting pbufs 
	SPB(tcp_service_give_back(peer, NULL);)

	return tcp_service_check_shutdown(peer)?
		ERR_OK:
		cbuf_tcp_send(peer);
#else
	tcp_service_check_shutdown(peer);
	return ERR_OK;
#endif
}

static err_t tcp_service_poll (void* svc, struct tcp_pcb* pcb)
{ 
	LWIP_UNUSED_ARG(pcb);
	tcpservice_t* service = (tcpservice_t*)svc;

	if (service->cb_poll)
		service->cb_poll(service);

#if 0 // not here
	// feed user with awaiting pbufs 
	SPB(tcp_service_give_back(service, NULL);)

	// trigger send buffer if needed
	cbuf_tcp_send(service);
#endif
	return ERR_OK;
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
	
#if STOREPBUF
	peer->pbuf = NULL;
	peer->pbuf_taken = 0;
#endif
	
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
	if (peer->cb_established)
		peer->cb_established(peer);
		
	return ERR_OK;
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

void tcp_service_close (tcpservice_t* s)
{
	if (s->tcp)
	{
		s->is_closing = true;
		if (s->cb_closing)
			s->cb_closing(s);
		tcp_service_check_shutdown(s);
	}
}
