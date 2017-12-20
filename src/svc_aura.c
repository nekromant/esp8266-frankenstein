#include "tcpservice.h"
#include "env.h"
#include "main.h"

#define AURA_BUFFER_SIZE 32
static int connected = 0;
static size_t aura_recv (tcpservice_t* peer, const char* data, size_t len)
{
	/* Write data to aura */
	return tcp_service_write(peer, data, len);
}

static void aura_ack (tcpservice_t* peer, size_t len)
{
	connected++;
	tcp_service_allow_more(peer, len);
}

static tcpservice_t* aura_new_peer (tcpservice_t* listener)
{
	tcpservice_t* peer = tcp_service_init_new_peer_size(AURA_BUFFER_SIZE);
	if (peer)
	{
		peer->cb_recv = aura_recv;
		peer->cb_ack = aura_ack;
	}
	return peer;
}

static tcpservice_t aura_listener = TCP_SERVICE_LISTENER("aura", aura_new_peer);

int aura_start (int port)
{
	if (port <= 0)
		port = 10102;
	console_printf("Starting aura rpc server @ port %d\n", port);
	return tcp_service_install("aura", &aura_listener, port);
}

static int  do_aura(int argc, const char* const* argv)
{
	if (strcmp(argv[1], "start") == 0)
		return aura_start(-1);
	else
		console_printf("aura: invalid command '%s'\n", argv[1]);

	return 0;
}

CONSOLE_CMD(aura, 2, 2,
	    do_aura, NULL, NULL,
	    "start/stop aura rpc server"
	    HELPSTR_NEWLINE "aura start - start aura service");
