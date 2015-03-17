#ifndef _TCPSERVICE_H_
#define _TCPSERVICE_H_

#include <lwip/tcp.h>

#include "cbtools.h"

typedef struct tcpservice_s tcpservice_t;

struct tcpservice_s
{
	const char* name;
	struct tcp_pcb* tcp;
	bool is_closing;

	// listener args&callbacks
	tcpservice_t* (*get_new_peer) (tcpservice_t* s);

	// peer args&callbacks
	cb_t send_buffer;
	void (*cb_established) (tcpservice_t* s);
	void (*cb_closing) (tcpservice_t* s);
	void (*cb_recv) (tcpservice_t* s, const char* data, size_t len);
	err_t (*cb_poll) (tcpservice_t* s);
	err_t (*cb_ack) (tcpservice_t* s);
	
	//XXX add generic pointer for per-client data?
	//XXX (like allowing multi-telnet)
};

#define TCP_SERVICE_VOID()			\
{						\
	.name = NULL;				\
	.tcp = NULL;				\
	.is_closing = false;			\
	.send_buffer = CB_INIT(NULL, 0),	\
	.cb_accepted = NULL,			\
	.cb_closing = NULL,			\
	.cb_recv = NULL,			\
	.cb_poll = NULL,			\
	.cb_ack = NULL,				\
}

void tcp_log_err (err_t err);
err_t cb_tcp_send (cb_t* cb, struct tcp_pcb *pcb);

// install tcp service on port
int tcp_service_install (const char* name, tcpservice_t* s, int port);

// connect to remote server:port
void tcp_service_connect_peer (tcpservice_t* s, const char* host, int port);

//
void tcp_service_close (tcpservice_t* s);

#endif // _TCPSERVICE_H_
