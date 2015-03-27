
/// TODO virer await, utiliser pbuf->next?

#include <stdlib.h>
#include <stdarg.h>

#include "console.h"
#include "tcpservice.h"
#include "cbuf.h"

//XXX a configuration value for this?
#define TCPVERBERR 2 // 0:silent 1:fatal-only 2:all

static void tcp_service_error (void* svc, err_t err)
{
#if TCPVERBERR
	// verbose
	tcpservice_t* peer = (tcpservice_t*)svc;
	static const char* lwip_err_msg [] =
		{
			"OK", "MEM", "BUF", "TIMEOUT", "ROUTE", "INPROGRESS", "INVAL",
#if TCPVERBERR > 1
			"WBLOCK", "ABORT", "RESET", "CLOSED", "INARG", "INUSE", "IFERR", "ISCONN"
#endif // TCPVERBERR > 1
		};
	LOGSERIAL(ERR_IS_FATAL(err)? LOG_ERR: LOG_WARN, "TCP(%s): %serror %d (%s)",
		peer->name,
		ERR_IS_FATAL(err)? "fatal ": "",
		(int)err,
		err < 0 && -err < sizeof(lwip_err_msg) / sizeof(lwip_err_msg[0])? lwip_err_msg[-err]: "?");
#endif // TCPVERBERR > 0
	if (ERR_IS_FATAL(err))
		tcp_service_close(peer);
}

// tcp_send() is static:
// tcp_write() cannot be called by user's callbacks (it hangs lwip)
// trigger write-from-circular-buffer
static err_t cbuf_tcp_send (tcpservice_t* tcp)
{
	err_t err = ERR_OK;
	char* data;
	
	size_t sendsize = cbuf_read_ptr(&tcp->send_buffer, &data, tcp_sndbuf(tcp->tcp) /* = sendmax */);
	if (   sendsize
	    && (   (err = tcp_write(tcp->tcp, data, sendsize, /*tcpflags=0=PUSH,NOCOPY*/0)) != ERR_OK
	        || (err = tcp_output(tcp->tcp)) != ERR_OK))
	{
		tcp_service_error(tcp, err);
	}

	return err;
}

#if STOREPBUF
#define D(x...) SERIAL_PRINTF(x)

static void tcp_service_give_back (tcpservice_t* peer, struct pbuf *pbuf)
{
	while (!cbuf_is_empty(&peer->recvwait.pbufs))
	{
		// try first to give already-received data back to user
		typeof(pbuf) wpbuf = *((typeof(pbuf)*)cbuf_peek(&peer->recvwait.pbufs));
D("peek %d / %d\n", peer->recvwait.swallowed, wpbuf->len);
		size_t user_acked = peer->cb_recv(peer, ((char*)wpbuf->payload) + peer->recvwait.swallowed, wpbuf->len - peer->recvwait.swallowed);
D("uack %d, remain %d\n", user_acked, wpbuf->len - peer->recvwait.swallowed - user_acked);
	//	if (user_acked)
	//		tcp_recved(peer->tcp, user_acked);
if (peer->recvwait.swallowed + user_acked > wpbuf->len) SERIAL_PRINTF("KRAK\n");
		if ((peer->recvwait.swallowed += user_acked) == wpbuf->len)
		{
D("apbfree\n");
			// this pbuf is now empty, we must free it
			pbuf_free(wpbuf);
	tcp_recved(peer->tcp, wpbuf->len);
			// reset swallowed
			peer->recvwait.swallowed = 0;
			// remove this pbuf pointer from circular buffer
			if (cbuf_forget(&peer->recvwait.pbufs, sizeof(pbuf)) != sizeof(pbuf))
				SERIAL_PRINTF("forget internal error forget\n"); //XXX remove this test
		}
		else
			// user is stuffed
			break;
	}
	
	if (pbuf)
	{
		// a new pbuf has come
		size_t swallowed = 0;
		
		if (cbuf_is_empty(&peer->recvwait.pbufs))
		{
			// try to give pbuf to user
			swallowed = peer->cb_recv(peer, pbuf->payload, pbuf->len);
//D("npback %d <= %d\n", swallowed, pbuf->len);
		//	if (swallowed)
		//		tcp_recved(peer->tcp, swallowed);
if (swallowed > pbuf->len) SERIAL_PRINTF("KRAK2\n");
			if (swallowed == pbuf->len)
			{
//D("npbfree\n");
				// user was big eater
				pbuf_free(pbuf);
		tcp_recved(peer->tcp, pbuf->len);
				pbuf = NULL;
				swallowed = 0;
			}
		}
		
		if (pbuf)
		{
			// user is really stuffed, store pbuf
			if (cbuf_write(&peer->recvwait.pbufs, &pbuf, sizeof(pbuf)) != sizeof(pbuf))
				SERIAL_PRINTF("ALERT awaiting pbuf buffer too small (PBUF_WAIT_SIZE_LOG2)!\n");
			else if (swallowed)
				// !!swallowed means cbuf was empty and this pbuf is partially swallowed
				peer->recvwait.swallowed = swallowed;
D("stored pbuf, swallowed %d\n", swallowed);
		}
	}
//D("out\n");
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
		while (pbuf)
		{
			tcp_service_give_back(peer, pbuf);
			pbuf = pbuf->next;
		}
#else		
		size_t all = 0;
		while (pbuf)
		{
			size_t acked = peer->cb_recv(peer, pbuf->payload, pbuf->len);
			if (acked != pbuf->len)
				SERIAL_PRINTF("OOOPS\n");
			all += pbuf->len;
			pbuf = pbuf->next;
		}
		tcp_recved(peer->tcp, all);
#endif
		return cbuf_tcp_send(peer);
	}
	else
	{
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
		s->tcp = NULL;
		if (s->cb_cleanup)
			s->cb_cleanup(s);
		return true;
	}
	return false;
}

static err_t tcp_service_ack (void *svc, struct tcp_pcb *pcb, u16_t len)
{
	tcpservice_t* peer = (tcpservice_t*)svc;

	if (len)
	{
		cbuf_ack(&peer->send_buffer, len);
		if (peer->cb_ack)
			peer->cb_ack(peer);
	}

#if 1
	// feed user with awaiting pbufs 
	tcp_service_give_back(peer, NULL);

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
	tcp_service_give_back(service, NULL);

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

#if STOREPBUF
#define PBUF_WAIT_SIZE_LOG2	8	// 8: 256 bytes
#define PBUF_WAIT_NUM	((1 << (PBUF_WAIT_SIZE_LOG2)) / sizeof(pbuf*))

	char* awaitpbuf = (char*)os_malloc(1 << PBUF_WAIT_SIZE_LOG2);
	if (!awaitpbuf)
		return ERR_MEM;
#endif // STOREPBUF

	tcpservice_t* peer = listener->cb_get_new_peer(listener);
	if (!peer)
	{
		os_free(awaitpbuf);
		return ERR_MEM; //XXX handle this better
	}

	peer->tcp = peer_pcb;
	peer->is_closing = 0;

#if STOREPBUF
	peer->recvwait.pbuf = awaitpbuf;
	peer->recvwait.swallowed = 0;
	cbuf_init(&peer->recvwait.pbufs, peer->recvwait.pbuf, PBUF_WAIT_SIZE_LOG2);
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
#if STOREPBUF
		os_free(s->recvwait.pbuf);
#endif
		tcp_service_check_shutdown(s);
	}
}
