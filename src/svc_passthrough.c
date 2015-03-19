
#include "osapi.h"
#include "driver/uart.h"

#include "tcpservice.h"

// default log2(buffer size) for a passthrough client
// tried 11/2KB but too small for large help string
#define PASSTHROUGH_SEND_BUFFER_SIZE_LOG2_DEFAULT	8 // log2(buffersize)

///////////////////////////////////////////////////////////

// callbacks for tcp service

static tcpservice_t* passthrough_new_peer (tcpservice_t* s);
static void passthrough_established (tcpservice_t* s);
static void passthrough_closing (tcpservice_t* s);
static void passthrough_recv (tcpservice_t* s, const char* data, size_t len);
static void passthrough_cleanup (tcpservice_t* s);

///////////////////////////////////////////////////////////
// static data (small ram footprint)

// tcp server only, takes no space
// (the "socketserver" awaiting for incoming request only)
static tcpservice_t passthrough_listener = TCP_SERVICE_LISTENER("passthrough listener", passthrough_new_peer);

tcpservice_t* svc_passthrough = NULL;

///////////////////////////////////////////////////////////

static tcpservice_t* passthrough_new_peer (tcpservice_t* ts)
{
	if (svc_passthrough)
		return NULL;
	if (!ts)
		return NULL;
	ts->sendbuf = (char*)os_malloc(1 << (PASSTHROUGH_SEND_BUFFER_SIZE_LOG2_DEFAULT));
	if (!ts->sendbuf)
	{
		os_free(ts);
		return NULL;
	}
	
	ts->name = "passthrough";
	cb_init(&ts->send_buffer, ts->sendbuf, PASSTHROUGH_SEND_BUFFER_SIZE_LOG2_DEFAULT);
	ts->get_new_peer = NULL;
	ts->cb_established = passthrough_established;
	ts->cb_closing = passthrough_closing;
	ts->cb_recv = passthrough_recv;
	ts->cb_poll = NULL;
	ts->cb_ack = NULL;
	ts->cb_cleanup = passthrough_cleanup;

	svc_passthrough = ts;

	return ts;
}

static void passthrough_established (tcpservice_t* s)
{
	// status led ? blinking ?
}

static void passthrough_closing (tcpservice_t* ts)
{
	svc_passthrough = NULL;
}

static void passthrough_cleanup (tcpservice_t* ts)
{
	if (ts->sendbuf)
		os_free(ts->sendbuf);
	os_free(ts);
}

static void passthrough_recv (tcpservice_t* s, const char* q, size_t len)
{
	const char* p = q;
	while (len--)
		uart0_tx_one_char(*p++);
}

void passthrough_send (char c)
{
	// this is slow (char by char, and active send loop)
	// todo: 
	// handle multiple byte receive, put to a local circular buffer
	// add a tx empty buffer interruption (seems possible) and deal with our circular buffer to fill it

	if (svc_passthrough)
	{
		char *p;
		if (cb_write_ptr(&svc_passthrough->send_buffer, &p, 1))
		{
			*p = c;
			return;
		}
		console_printf("passthrough: send overrun\n");
	}
	
	//XXX here: error (send buffer full, or svc not initialized)
	//XXX led status ?
}

int passthrough_start (int port)
{
	if (svc_passthrough)
	{
		console_printf("passthrough already started\n");
		return -1;
	}
	if (port <= 0)
		port = 10101;
	return tcp_service_install("passthrough", &passthrough_listener, port);
}

int passthrough_stop (void)
{
	if (passthrough_listener.tcp)
		tcp_service_close(&passthrough_listener);
	return 0;
}

static int do_passthrough (int argc, const char* const* argv)
{
	if (strcmp(argv[1], "start") == 0)
		return passthrough_start(-1);
	else if (strcmp(argv[1], "stop") == 0)
		passthrough_stop();
	else if (strcmp(argv[1], "quit") == 0)
	{
		if (svc_passthrough)
			tcp_service_close(svc_passthrough);
	}
	else
		return -1;
	
	return 0;
}


CONSOLE_CMD(passthrough, 2, 2, 
	    do_passthrough, NULL, NULL, 
	    "start/stop passthrough server"
	    HELPSTR_NEWLINE "passthrough start - start service"
	    HELPSTR_NEWLINE "passthrough stop  - stop service"
	    HELPSTR_NEWLINE "passthrough quit  - drop current client");
