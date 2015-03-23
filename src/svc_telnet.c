
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

// default log2(buffer size) for a telnet client
// tried 11/2KB but too small for large help string
#define TELNET_SEND_BUFFER_SIZE_LOG2_DEFAULT	12 // 12: 4KB

///////////////////////////////////////////////////////////

// telnet state structure
typedef struct telnet_state_s
{
	int state;
	int idle;
	int max_idle;
} telnet_state_t;

// embed tcp_service in a more global telnet specific structure
typedef struct telnet_service_s
{
	tcpservice_t peer;	// tcp service - COMES FIRST HERE
	telnet_state_t state;	// telnet state
} telnet_service_t;

#define TS(s)		((telnet_service_t*)(s))
	
///////////////////////////////////////////////////////////
// callbacks for tcp service

static tcpservice_t* telnet_new_peer (tcpservice_t* s);
static void telnet_established (tcpservice_t* s);
static void telnet_closing (tcpservice_t* s);
static void telnet_recv (tcpservice_t* s, const char* data, size_t len);
static void telnet_poll (tcpservice_t* s);
static void telnet_cleanup (tcpservice_t* s);

///////////////////////////////////////////////////////////
// static data (small ram footprint)

// tcp server only, takes no space
// (the "socketserver" awaiting for incoming request only)
static tcpservice_t telnet_listener = TCP_SERVICE_LISTENER("telnet listener", telnet_new_peer);

// the current talking telnet peer, which is a hack
static telnet_service_t* current_telnet = NULL;
#define CURRENT(s)	do microrl_set_echo(!(current_telnet = TS(s))); while (0)

///////////////////////////////////////////////////////////

static int telnet_printf (const char *fmt, ...)
{

// something is wrong with telnet printf, or global console_printf

	int ret;

	va_list ap;
	va_start(ap, fmt);
	if (current_telnet && current_telnet->peer.tcp)
		ret = cb_vprintf(&current_telnet->peer.send_buffer, fmt, ap);
	else
	{
		vsnprintf(sprintbuf, SPRINTBUFSIZE, fmt, ap);
		ret = SERIAL_PRINTF(sprintbuf);
	}
	va_end(ap);

	return ret;
}

static tcpservice_t* telnet_new_peer (tcpservice_t* s)
{
	// allocate a new telnet service structure
	telnet_service_t* ts = (telnet_service_t*)os_malloc(sizeof(telnet_service_t));
	if (!ts)
		return NULL;
	ts->peer.sendbuf = (char*)os_malloc(1 << (TELNET_SEND_BUFFER_SIZE_LOG2_DEFAULT));
	if (!ts->peer.sendbuf)
	{
		os_free(ts);
		return NULL;
	}
	
	ts->peer.name = "telnet";
	cb_init(&ts->peer.send_buffer, ts->peer.sendbuf, TELNET_SEND_BUFFER_SIZE_LOG2_DEFAULT);
	ts->peer.get_new_peer = NULL;
	ts->peer.cb_established = telnet_established;
	ts->peer.cb_closing = telnet_closing;
	ts->peer.cb_recv = telnet_recv;
	ts->peer.cb_poll = telnet_poll;
	ts->peer.cb_ack = NULL;
	ts->peer.cb_cleanup = telnet_cleanup;

	ts->state.state = STATE_NORMAL;
	ts->state.idle = 0;
	ts->state.max_idle = 60;
	const char *tmp = env_get("telnet-drop");
	if (tmp)
		ts->state.max_idle = atoi(tmp);
	return &ts->peer;
}

static void telnet_established (tcpservice_t* s)
{
	console_printf("\ntelnet: Incoming connection, switching to telnet console...\n"); 
	console_printf = telnet_printf;

	/* Send in a welcome message */
	CURRENT(s);
	console_printf("Welcome to %s!\n", env_get("hostname"));
	console_printf("Press enter to activate this console\n");
}

static void telnet_closing (tcpservice_t* s)
{
	CURRENT(NULL);
}

static void telnet_cleanup (tcpservice_t* s)
{
	telnet_service_t* ts = TS(s);
	if (ts->peer.sendbuf)
		os_free(ts->peer.sendbuf);
	os_free(ts);
}

static void telnet_poll (tcpservice_t* s)
{ 
	telnet_service_t* ts = TS(s);
	if (ts->state.max_idle != -1 && ++ts->state.idle >= ts->state.max_idle)
	{
		CURRENT(ts);
		telnet_printf("\nYou have been idle for %d seconds, goodbye\n", ts->state.max_idle);
		tcp_service_close(s);
	}
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
	telnet_service_t* ts = TS(s);
	
	CURRENT(ts);
	
	ts->state.idle = 0;
	while (len > 0)
	{
		char c = *q++;
		--len;
	       
		switch (ts->state.state) {
		case STATE_IAC:
			if(c == TELNET_IAC) {
				console_insert(c);
				ts->state.state = STATE_NORMAL;
			} else {
				switch(c) {
				case TELNET_WILL:
					ts->state.state = STATE_WILL;
					break;
				case TELNET_WONT:
					ts->state.state = STATE_WONT;
					break;
				case TELNET_DO:
					ts->state.state = STATE_DO;
					break;
				case TELNET_DONT:
					ts->state.state = STATE_DONT;
					break;
				default:
					ts->state.state = STATE_NORMAL;
					break;
				}
			}
			break;

		case STATE_WILL:
			/* Reply with a DONT */
			sendopt(s, TELNET_DONT, c);
			ts->state.state = STATE_NORMAL;
			break;
		case STATE_WONT:
			/* Reply with a DONT */
			sendopt(s, TELNET_DONT, c);
			ts->state.state = STATE_NORMAL;
			break;
		case STATE_DO:
			/* Reply with a WONT */
			sendopt(s, TELNET_WONT, c);
			ts->state.state = STATE_NORMAL;
			break;
		case STATE_DONT:
			/* Reply with a WONT */
			sendopt(s, TELNET_WONT, c);
			ts->state.state = STATE_NORMAL;
			break;
		case STATE_NORMAL:
			if(c == TELNET_IAC) {
				ts->state.state = STATE_IAC;
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

int telnet_stop (void)
{
	if (telnet_listener.tcp)
		tcp_service_close(&telnet_listener);
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
			tcp_service_close(&current_telnet->peer);
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
