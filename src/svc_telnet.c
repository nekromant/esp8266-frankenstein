
// TODO: include telnet_state_t in send_buffer so to remove the need of telnet_service_s

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

#define STATE_NORMAL 0
#define STATE_IAC    1
#define STATE_WILL   2
#define STATE_WONT   3
#define STATE_DO     4
#define STATE_DONT   5
#define STATE_CLOSE  6

#define IAC_SUPPRESS_ECHO 0x2D

#include "tcpservice.h"
#include "microrl.h"
#include "env.h"
#include "main.h"

//XXX something has to be done with cmd-help-like cases
// which want to send a big buffer at once
#define TELNET_SEND_BUFFER_SIZE			3500

///////////////////////////////////////////////////////////

// telnet state structure
typedef struct telnet_state_s
{
	int state;
	int idle;
	int max_idle;
} telnet_state_t;

#define state(s)	((telnet_state_t*)((s)->sendbuf + (s)->send_buffer.size))

///////////////////////////////////////////////////////////
// callbacks for tcp service

static tcpservice_t* telnet_new_peer (tcpservice_t* s);
static err_t telnet_established (tcpservice_t* s);
static void telnet_closing (tcpservice_t* s);
static size_t telnet_recv (tcpservice_t* s, const char* data, size_t len);
static void telnet_poll (tcpservice_t* s);

///////////////////////////////////////////////////////////
// static data (small ram footprint)

// tcp server only, takes no space
// (the "socketserver" awaiting for incoming request only)
static tcpservice_t telnet_listener = TCP_SERVICE_LISTENER("telnet listener", telnet_new_peer);

// the current talking telnet peer
static tcpservice_t*	current_telnet = NULL;
#define CURRENT(s)	do microrl_set_echo(!(current_telnet = (s))); while (0)

///////////////////////////////////////////////////////////

static int telnet_printf (const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	if (current_telnet && !current_telnet->tcp)
		current_telnet = NULL;
	if (current_telnet)
		ret = cbuf_vprintf(&current_telnet->send_buffer, fmt, ap);
	else
	{
		vsnprintf(sprintbuf__, SPRINTBUFSIZE, fmt, ap);
		ret = SERIAL_PRINTF(sprintbuf__);
	}
	va_end(ap);

	return ret;
}

static tcpservice_t* telnet_new_peer (tcpservice_t* listener)
{
	// allocate send_buffer + telnet state structure
	char* sendbuf = (char*)os_malloc(TELNET_SEND_BUFFER_SIZE + sizeof(telnet_state_t));
	if (sendbuf == NULL)
		return NULL;
	// generic initialization with provided buffer
	tcpservice_t* peer = tcp_service_init_new_peer_sendbuf_size(sendbuf, TELNET_SEND_BUFFER_SIZE);
	if (!peer)
	{
		os_free(sendbuf);
		return NULL;
	}

	peer->cb_established = telnet_established;
	peer->cb_closing = telnet_closing;
	peer->cb_recv = telnet_recv;
	peer->cb_poll = telnet_poll;
	tcp_service_set_poll_ms(peer, 1000);

	const char* tmp = env_get("telnet-drop");
	state(peer)->state = STATE_NORMAL;
	state(peer)->idle = 0;
	state(peer)->max_idle = tmp? atoi(tmp): 60;


	return peer;
}

static err_t telnet_established (tcpservice_t* s)
{
	console_printf("\ntelnet: Incoming connection, switching to telnet console...\n");
	console_printf = telnet_printf;

	/* Send in a welcome message */
	CURRENT(s);
	console_printf("Welcome to %s!\n", env_get("hostname"));
    console_auth_start();

	return ERR_OK;
}

static void telnet_closing (tcpservice_t* peer)
{
	CURRENT(NULL);
}

static void telnet_poll (tcpservice_t* peer)
{
	if (state(peer)->max_idle > 0 && ++state(peer)->idle >= state(peer)->max_idle)
	{
		CURRENT(peer);
		telnet_printf("\nYou have been idle for %d seconds, goodbye\n", state(peer)->max_idle);
		tcp_service_request_close(peer);
	}
}

int sendopt (tcpservice_t* s, u8_t option, u8_t value)
{
	char tmp[] = { TELNET_IAC, option, value, 0 };
	if (tcp_service_write(s, tmp, sizeof tmp) != sizeof tmp)
	{
		LOGSERIAL(LOG_ERR, "telnet out of buf");
		return -1;
	}
	return 0;
}

static size_t telnet_recv (tcpservice_t* ts, const char* q, size_t len)
{
	size_t ret = len;

	CURRENT(ts);

	state(ts)->idle = 0;
	while (len > 0)
	{
		char c = *q++;
		--len;

		switch (state(ts)->state) {
		case STATE_IAC:
			if(c == TELNET_IAC) {
				console_insert(c);
				state(ts)->state = STATE_NORMAL;
			} else {
				switch(c) {
				case TELNET_WILL:
					state(ts)->state = STATE_WILL;
					break;
				case TELNET_WONT:
					state(ts)->state = STATE_WONT;
					break;
				case TELNET_DO:
					state(ts)->state = STATE_DO;
					break;
				case TELNET_DONT:
					state(ts)->state = STATE_DONT;
					break;
				default:
					state(ts)->state = STATE_NORMAL;
					break;
				}
			}
			break;

		case STATE_WILL:
			/* Reply with a DONT */
			sendopt(ts, TELNET_DONT, c);
			state(ts)->state = STATE_NORMAL;
			break;
		case STATE_WONT:
			/* Reply with a DONT */
			sendopt(ts, TELNET_DONT, c);
			state(ts)->state = STATE_NORMAL;
			break;
		case STATE_DO:
			/* Reply with a WONT */
			sendopt(ts, TELNET_WONT, c);
			state(ts)->state = STATE_NORMAL;
			break;
		case STATE_DONT:
			/* Reply with a WONT */
			sendopt(ts, TELNET_WONT, c);
			state(ts)->state = STATE_NORMAL;
			break;
		case STATE_NORMAL:
			if(c == TELNET_IAC)
				state(ts)->state = STATE_IAC;
			else
				console_insert(c);
			break;
		}
	}

	return ret;
}

int telnet_start (int port)
{
	if (port <= 0)
	{
		const char *tmp = env_get("telnet-port");
		if (tmp)
			port = atoi(tmp);
	}
	if (port <= 0)
		port = 23;
	return tcp_service_install("telnet", &telnet_listener, port);
}

int telnet_stop (void)
{
	if (telnet_listener.tcp)
		tcp_service_request_close(&telnet_listener);
	return 0;
}

static int  do_telnet(int argc, const char* const* argv)
{
	if (strcmp(argv[1], "start") == 0)
		return telnet_start(-1);
	else if (strcmp(argv[1], "stop") == 0)
		telnet_stop();
	else if (strcmp(argv[1], "quit") == 0)
	{
		if (current_telnet)
		{
			console_printf("telnet: See you!\n");
			tcp_service_request_close(current_telnet);
		}
	}
	else
		console_printf("telnet: invalid command '%s'\n", argv[1]);

	return 0;
}


CONSOLE_CMD(telnet, 2, 2,
	    do_telnet, NULL, NULL,
	    "start/stop telnet server"
	    HELPSTR_NEWLINE "telnet start - start service"
	    HELPSTR_NEWLINE "telnet stop  - stop service"
	    HELPSTR_NEWLINE "telnet quit  - drop current client");
