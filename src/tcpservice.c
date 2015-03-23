
#include <stdlib.h>
#include <stdarg.h>

#include "console.h"
#include "tcpservice.h"
#include "cbuf.h"

void tcp_log_err (err_t err)
{
	LOGSERIAL(LOG_ERR, "TCP: Fatal error %d(%s)", (int)err, lwip_strerr(err));
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
		tcp_log_err(err);
	}

	return err;
}

static err_t tcp_service_receive (void* svc, struct tcp_pcb *pcb, struct pbuf *pbuf, err_t err)
{
	tcpservice_t* peer = (tcpservice_t*)svc;

	if (err == ERR_OK && pbuf != NULL)
	{
		tcp_recved(peer->tcp, pbuf->tot_len);
		peer->cb_recv(peer, pbuf->payload, pbuf->tot_len);
		pbuf_free(pbuf);
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
	
	return tcp_service_check_shutdown(peer)?
		ERR_OK:
		cbuf_tcp_send(peer);
}

static void tcp_service_error (void* svc, err_t err)
{
	LWIP_UNUSED_ARG(svc);
	LOGSERIAL(LOG_ERR, "TCP: Fatal error %d(%s)", (int)err, lwip_strerr(err));
}

static err_t tcp_service_poll (void* svc, struct tcp_pcb* pcb)
{ 
	LWIP_UNUSED_ARG(pcb);
	tcpservice_t* service = (tcpservice_t*)svc;

	if (service->cb_poll)
		service->cb_poll(service);

	// trigger send buffer if needed
	cbuf_tcp_send(service);
		
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
	
	tcp_setprio(peer->tcp, TCP_PRIO_MIN); //XXX???
	tcp_recv(peer->tcp, tcp_service_receive);
	tcp_err(peer->tcp, tcp_service_error);
	tcp_poll(peer->tcp, tcp_service_poll, 4); //every two seconds of inactivity of the TCP connection
	tcp_sent(peer->tcp, tcp_service_ack);
	
	// peer/tcp_pcb association
	tcp_arg(peer->tcp, peer);
	
#if 0
	SERIAL_PRINTF("%s: nagle=%d\n", peer->name, tcp_nagle_disabled(peer->tcp));
	tcp_nagle_disable(peer->tcp);
	SERIAL_PRINTF("%s: nagle=%d\n", peer->name, tcp_nagle_disabled(peer->tcp));
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
