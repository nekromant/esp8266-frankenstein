#ifndef _TCPSERVICE_H_
#define _TCPSERVICE_H_

#include <lwip/tcp.h>

#include "cbuftools.h"

typedef struct tcpservice_s tcpservice_t;

struct tcpservice_s
{
	const char* name;
	struct tcp_pcb* tcp;
	bool is_closing;

	// circular buffer
	char* sendbuf;
	cbuf_t send_buffer;

	// listener callbacks
	tcpservice_t* (*cb_get_new_peer) (tcpservice_t* s);

	// peer callbacks
	void (*cb_established) (tcpservice_t* s);
	void (*cb_closing) (tcpservice_t* s);
	void (*cb_recv) (tcpservice_t* s, const char* data, size_t len);
	void (*cb_poll) (tcpservice_t* s);
	void (*cb_ack) (tcpservice_t* s);
	void (*cb_cleanup) (tcpservice_t* s);
};

#define TCP_SERVICE_LISTENER(nameptr, cb_new_peer) \
{						\
	.name = (nameptr),			\
	.tcp = NULL,				\
	.is_closing = false,			\
	.sendbuf = NULL,			\
	.send_buffer = CBUF_INIT(NULL, 0),	\
	.cb_get_new_peer = (cb_new_peer),	\
	.cb_established = NULL,			\
	.cb_closing = NULL,			\
	.cb_recv = NULL,			\
	.cb_poll = NULL,			\
	.cb_ack = NULL,				\
	.cb_cleanup = NULL,			\
}

void tcp_log_err (err_t err);

// install tcp service on port
int tcp_service_install (const char* name, tcpservice_t* s, int port);

// connect to remote server:port
void tcp_service_connect_peer (tcpservice_t* s, const char* host, int port);

//
void tcp_service_close (tcpservice_t* s);

#endif // _TCPSERVICE_H_
