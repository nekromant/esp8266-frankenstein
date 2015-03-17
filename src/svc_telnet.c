
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

static tcpservice_t* telnet_new_peer (tcpservice_t* s);
static void telnet_established (tcpservice_t* s);
static void telnet_closing (tcpservice_t* s);
static void telnet_recv (tcpservice_t* s, const char* data, size_t len);
static err_t telnet_poll (tcpservice_t* s);

#define TELNET_BUFFER_SIZE_LOG2	13	// 13: 8KB
static char telnetbuf [1 << (TELNET_BUFFER_SIZE_LOG2)];

static tcpservice_t telnet_listener =
{
	.name = "telnet listener",
	.tcp = NULL,
	.is_closing = false,
	.send_buffer = CB_INIT(NULL, 0),
	.get_new_peer = telnet_new_peer,
	.cb_established = NULL,
	.cb_closing = NULL,
	.cb_recv = NULL,
	.cb_poll = NULL,
	.cb_ack = NULL,
};

static tcpservice_t telnet_peer =
{
	.name = "telnet peer",
	.tcp = NULL,
	.is_closing = false,
	.send_buffer = CB_INIT(telnetbuf, TELNET_BUFFER_SIZE_LOG2),
	.get_new_peer = NULL,
	.cb_established = telnet_established,
	.cb_closing = telnet_closing,
	.cb_recv = telnet_recv,
	.cb_poll = telnet_poll,
	.cb_ack = NULL,
};

// XXX limited to one client only
static struct telnet_data
{
	int state;
	int idle;
	int max_idle;
	//void *prev_printf;
	printf_f prev_printf;
} ts;

static int telnet_printf (const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = cb_vprintf(&telnet_peer.send_buffer, fmt, ap);
	va_end(ap);
	return ret;
}

static tcpservice_t* telnet_new_peer (tcpservice_t* s)
{
	if (telnet_peer.tcp)
	{
		return NULL;
//		return &telnet_deny;
//		/* Someone already connected */
//		//static const char busy[] = "Sorry, but this telnet console is already in use by someone\n";
//		//TCP_WRITE(cli_pcb, busy, strlen(busy), 0);
//		
	}
	
	return &telnet_peer;
}

static void telnet_established (tcpservice_t* s)
{
	console_printf("\ntelnet: Incoming connection, switching to telnet console...\n"); 
	ts.prev_printf = console_printf;
	console_printf = telnet_printf;


	/* Send in a welcome message */
	console_printf("Welcome to %s!\n", env_get("hostname"));
	console_printf("Press enter to activate this console\n");

	ts.state = STATE_NORMAL;
	ts.idle = 0;
	ts.max_idle = 60;

	const char *tmp = env_get("telnet-drop");
	if (tmp)
		ts.max_idle = atoi(tmp);
}

static void telnet_closing (tcpservice_t* s)
{
	console_printf("bouh\n");
}

static err_t telnet_poll (tcpservice_t* s)
{ 
	if (ts.max_idle != -1 && ++ts.idle >= ts.max_idle)
	{
		telnet_printf("\nYou have been idle for a while, goodbye\n");
		//XXXtcp_service_close(s);
		return ERR_TIMEOUT;
	}
	return ERR_OK;
}

int sendopt (tcpservice_t* s, u8_t option, u8_t value)
{
	char tmp[4];
	tmp[0] = TELNET_IAC;
	tmp[1] = option;
	tmp[2] = value;
	tmp[3] = 0;
	return cb_write(&s->send_buffer, tmp, 4) == 4? 0: -1;
}

static void telnet_recv (tcpservice_t* s, const char* q, size_t len)
{
	ts.idle = 0;
	while (len > 0)
	{
		char c = *q++;
		--len;
	       
		switch (ts.state) {
		case STATE_IAC:
			if(c == TELNET_IAC) {
				console_insert(c);
				ts.state = STATE_NORMAL;
			} else {
				switch(c) {
				case TELNET_WILL:
					ts.state = STATE_WILL;
					break;
				case TELNET_WONT:
					ts.state = STATE_WONT;
					break;
				case TELNET_DO:
					ts.state = STATE_DO;
					break;
				case TELNET_DONT:
					ts.state = STATE_DONT;
					break;
				default:
					ts.state = STATE_NORMAL;
					break;
				}
			}
			break;

		case STATE_WILL:
			/* Reply with a DONT */
			sendopt(s, TELNET_DONT, c);
			ts.state = STATE_NORMAL;
			break;
		case STATE_WONT:
			/* Reply with a DONT */
			sendopt(s, TELNET_DONT, c);
			ts.state = STATE_NORMAL;
			break;
		case STATE_DO:
			/* Reply with a WONT */
			sendopt(s, TELNET_WONT, c);
			ts.state = STATE_NORMAL;
			break;
		case STATE_DONT:
			/* Reply with a WONT */
			sendopt(s, TELNET_WONT, c);
			ts.state = STATE_NORMAL;
			break;
		case STATE_NORMAL:
			if(c == TELNET_IAC) {
				ts.state = STATE_IAC;
			} else {
				console_insert(c);
			}
			break;
		}
	}
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

static int  do_telnet(int argc, const char* const* argv)
{
	if (strcmp(argv[1], "start") == 0)
		return telnet_start(-1);
#if 0
	else if (strcmp(argv[1], "stop") == 0)
		telnet_stop();
#endif
	else if (telnet_peer.tcp && strcmp(argv[1], "quit") == 0)
	{
		console_printf("telnet: See you!\n");
		tcp_service_close(&telnet_peer);
	}
	else
		console_printf("telnet: invalid command '%s'\n", argv[1]);
	
	return 0;
}


CONSOLE_CMD(telnet, 2, 2, 
	    do_telnet, NULL, NULL, 
	    "start/stop telnet server"
	    HELPSTR_NEWLINE "telnet start - start it"
	    HELPSTR_NEWLINE "telnet stop  - stop it"
	    HELPSTR_NEWLINE "telnet quit  - drop current client");
