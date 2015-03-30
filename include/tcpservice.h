#ifndef _TCPSERVICE_H_
#define _TCPSERVICE_H_

#include <lwip/tcp.h>

#include "cbuftools.h"

typedef struct pbuf pbuf_t;
typedef struct tcpservice_s tcpservice_t;

struct tcpservice_s
{
	const char* name;
	struct tcp_pcb* tcp;
	bool is_closing;

	// circular buffers
	char* sendbuf;		// user data (pointed to by send_buffer)
	cbuf_t send_buffer;	// user circular buffer

	// received buffers
	pbuf_t* pbuf;		// pbuf chain to process
	size_t pbuf_taken;	// already taken from first

	// listener callbacks
	tcpservice_t* (*cb_get_new_peer) (tcpservice_t* s);

	// peer callbacks
	err_t (*cb_established) (tcpservice_t* s);
	void (*cb_closing) (tcpservice_t* s);
	size_t (*cb_recv) (tcpservice_t* s, const char* data, size_t len);
	err_t (*cb_poll) (tcpservice_t* s);
	void (*cb_cleanup) (tcpservice_t* s);
};

#define TCP_SERVICE_LISTENER(nameptr, cb_new_peer) \
{						\
	.name = (nameptr),			\
	.tcp = NULL,				\
	.is_closing = false,			\
	.sendbuf = NULL,			\
	.send_buffer = CBUF_INIT(NULL, 0),	\
	.pbuf = NULL,				\
	.pbuf_taken = 0, 			\
	.cb_get_new_peer = (cb_new_peer),	\
	.cb_established = NULL,			\
	.cb_closing = NULL,			\
	.cb_recv = NULL,			\
	.cb_poll = NULL,			\
	.cb_cleanup = NULL,			\
}

void tcp_log_err (err_t err);

// install tcp service on port
int tcp_service_install (const char* name, tcpservice_t* s, int port);

// connect to remote server:port XXXtodo
void tcp_service_connect_peer (tcpservice_t* s, const char* host, int port);

// initialize/allocate new peer common fields
tcpservice_t* tcp_service_init_new_peer_sizelog2 (char sendbufsizelog2);

// initialize/allocate new peer common fields
tcpservice_t* tcp_service_init_new_peer_sendbuf_sizelog2 (char* sendbuf, char sendbufsizelog2);

// gracefully close connection
void tcp_service_close (tcpservice_t* s);

#endif // _TCPSERVICE_H_
