#ifndef _TCPSERVICE_H_
#define _TCPSERVICE_H_

#include <lwip/tcp.h>

#include "cbuftools.h"

typedef struct pbuf pbuf_t;
typedef struct tcpservice_s tcpservice_t;

// some callbacks info:
// ---------
// cb_recv() feeds user with received wlan data.
// 
// pbufs are always released after calling this cb (otherwise
// link level get stuffed and at some time, all new packets are
// discarded, leading to tcp freeze because received acks are
// not updated).
// so user must be hungry enough, meaning: if data are stored,
// receive buffer must be big enough (something like TCP_WND).
// user must *always* return how much data is swallowed.  any
// value but 0 is allowed. user cb keeps beeing called until
// all received data is swallowed. if the user cb is stuffed,
// it is of his responsibility to yell or increase receive
// buffer. 0 must not be returned or data is lost.
// tcp receive window is raised just after this cb (meaning:
// data are swallowed, send more) *unless* user->cb_ack is
// defined, then user is responsible to increase the tcp
// receive window by calling tcp_service_allow_more() in this
// cb_ack callback.
// examples:
// - svc_telnet does not defined cb_ack, all data are processed
//   inside cb_recv.
// - svc_echo does define cb_ack, thus requesting to receive more
//   data only when already sent data are received by peer
//   (which is later, otherwise we are flooded: data would come
//   too fast, filling buffer before allowed to be sent back,
//   leading to lost data - because pbufs are always discarded
//   after cb_ack is called).
//   (the solution of retaining/chaining received pbuf is not good
//    because link layer discards all new packets when full, leading
//    to not updating received acks and freezing tcp)
// ---------
// cb_cleanup: called when connection is actually closed
// 	user must release sendbuf
//	if cb is NULL, buffer is automatically released
//	so it must be defined for static buffers
// ---------
// cb_closing: called when connection is about to close
// ---------

struct tcpservice_s
{
	const char* name;
	struct tcp_pcb* tcp;
	struct
	{
		char is_closing:	1;
		char verbose_error:	1;
		char nagle:		1;
	} bools;
	int poll_ms;

	// circular buffers
	char* sendbuf;		// user data (used by send_buffer)
	cbuf_t send_buffer;	// user circular send buffer

	// listener callbacks
	tcpservice_t* (*cb_get_new_peer) (tcpservice_t* s);

	// peer callbacks
	err_t (*cb_established) (tcpservice_t* s);
	void (*cb_closing) (tcpservice_t* s);
	size_t (*cb_recv) (tcpservice_t* s, const char* data, size_t len);
	void (*cb_ack) (tcpservice_t* s, size_t len);
	void (*cb_poll) (tcpservice_t* s);
	void (*cb_cleanup) (tcpservice_t* s);
};

#define TCP_SERVICE_LISTENER(nameptr, cb_new_peer) \
{						\
	.name = (nameptr),			\
	.tcp = NULL,				\
	.bools.is_closing = 0,			\
	.bools.verbose_error = 0,		\
	.bools.nagle = 1,			\
	.poll_ms = -1,				\
	.sendbuf = NULL,			\
	.send_buffer = CBUF_INIT(NULL, 0),	\
	.cb_get_new_peer = (cb_new_peer),	\
	.cb_established = NULL,			\
	.cb_closing = NULL,			\
	.cb_recv = NULL,			\
	.cb_ack = NULL,				\
	.cb_poll = NULL,			\
	.cb_cleanup = NULL,			\
}

#define tcp_service_allow_more(svc,len) do { tcp_recved((svc)->tcp, (len)); } while (0)
#define tcp_service_write(svc,data,len) (cbuf_write(&(svc)->send_buffer, (data), (len)))
#define tcp_service_verbose_error(svc,value) do { svc->bools.verbose_error = value; } while (0)

void tcp_log_err (err_t err);

// install tcp service on port
int tcp_service_install (const char* name, tcpservice_t* s, int port);

// connect to remote server:port XXXtodo
void tcp_service_connect_peer (tcpservice_t* s, const char* host, int port);

// initialize/allocate new peer common fields
tcpservice_t* tcp_service_init_new_peer_size (size_t sendbufsize);

// initialize/allocate new peer common fields
tcpservice_t* tcp_service_init_new_peer_sendbuf_size (char* sendbuf, size_t sendbufsize);

// option helpers in cb_get_new_peer()
#define tcp_service_set_poll_ms(s,pollms)	do { (s)->poll_ms = (pollms); } while (0)
#define tcp_service_disable_nagle(s)		do { (s)->bools.nagle = 0; } while (0)

// graceful request for close
void tcp_service_request_close (tcpservice_t* s);

#endif // _TCPSERVICE_H_
