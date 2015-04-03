
#include "tcpservice.h"
#include "env.h"

#define ECHO_SEND_BUFFER_SIZE_LOG2_DEFAULT	12 // 12: 4KB

///////////////////////////////////////////////////////////
// callbacks for tcp service

static tcpservice_t* echo_new_peer (tcpservice_t* s);
static size_t echo_recv (tcpservice_t* s, const char* data, size_t len);

///////////////////////////////////////////////////////////
// static data (small ram footprint)

// tcp server only, takes no space
// (the "socketserver" awaiting for incoming request only)
static tcpservice_t echo_listener = TCP_SERVICE_LISTENER("echo", echo_new_peer);

///////////////////////////////////////////////////////////

static tcpservice_t* echo_new_peer (tcpservice_t* listener)
{
	tcpservice_t* peer = tcp_service_init_new_peer_sizelog2(ECHO_SEND_BUFFER_SIZE_LOG2_DEFAULT);
	if (peer)
		peer->cb_recv = echo_recv;
	return peer;
}

static size_t echo_recv (tcpservice_t* peer, const char* data, size_t len)
{
	return cbuf_write(&peer->send_buffer, data, len);
}

int echo_start (int port)
{
	if (port <= 0)
		port = 10102;
	return tcp_service_install("echo", &echo_listener, port);
}

static int  do_echo(int argc, const char* const* argv)
{
	if (strcmp(argv[1], "start") == 0)
		return echo_start(-1);
	else
		console_printf("echo: invalid command '%s'\n", argv[1]);
	
	return 0;
}

CONSOLE_CMD(echo, 2, 2, 
	    do_echo, NULL, NULL, 
	    "start/stop tcp echo server"
	    HELPSTR_NEWLINE "echo start - start service");
