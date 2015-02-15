#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <generic/macros.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>

#include "telnet.h"
#include "env.h"

#define USE_TCPBUF 1

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

struct telnet_server
{
	struct tcp_pcb *server; 
	struct tcp_pcb *client; 
	int state;
	int idle;
	int max_idle;
	void *prev_printf;
};

static struct telnet_server *ts;

void tcp_log_err (err_t err)
{
	LOGSERIAL(LOG_ERR, "TCP: Fatal error %d(%s)", (int)err, lwip_strerr(err));
}

int telnet_printf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#if USE_TCPBUF

#include "tcpbuf.h"

tcpbuf telnetbuf = TCPBUF_INIT;

#define TCP_WRITE(pcb,data,len,flags) tcpbuf_write(&telnetbuf,pcb,data,len,flags)

int telnet_printf (const char *fmt, ...)
{
	if (!ts)
		return -1;
		
	va_list ap;
	va_start(ap, fmt);
	int ret = strbuf_vprintf(tcpbuf_wbuf(&telnetbuf), fmt, ap);
	va_end(ap);
	return ret;
}

#else // !USE_TCPBUF

#define TCP_WRITE(pcb,data,len,flags) tcp_write(pcb,data,len,flags)

int telnet_printf(const char *fmt, ...) 
{
	if (!ts)
		return -1;
	int ret; 
	va_list ap;
	static char p[256];
	va_start(ap, fmt);
	ret = vsnprintf(p, sizeof p, fmt, ap);
	va_end(ap);
	tcp_write(ts->client, p, ret, TCP_WRITE_FLAG_COPY);
	ts->idle = 0;
	return ret; 
}

#endif // !USE_TCPBUF

static void telnet_close(struct tcp_pcb *pcb)
{
	if (!pcb)
		return;
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_close(pcb);
	if (pcb == ts->client) { 
		ts->client = NULL;
		console_printf = ts->prev_printf;
		console_printf("\ntelnet: console restored\n");
		microrl_set_echo(1);
	}
}

void sendopt(u8_t option, u8_t value)
{
	char tmp[4];
	tmp[0] = TELNET_IAC;
	tmp[1] = option;
	tmp[2] = value;
	tmp[3] = 0;
	TCP_WRITE(ts->client, tmp, 4, 0);
}

static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char *q, c;
	int len;
	LWIP_UNUSED_ARG(arg);
	/* Reset idle timer */
	ts->idle = 0;
	
	if (err == ERR_OK && p != NULL)
	{
		tcp_recved(pcb, p->tot_len);
		q = p->payload;
		len = p->tot_len;
		while(len > 0)
		{
			c = *q++;
			--len;
	       
			switch (ts->state) {
			case STATE_IAC:
				if(c == TELNET_IAC) {
					console_insert(c);
					ts->state = STATE_NORMAL;
				} else {
					switch(c) {
					case TELNET_WILL:
						ts->state = STATE_WILL;
						break;
					case TELNET_WONT:
						ts->state = STATE_WONT;
						break;
					case TELNET_DO:
						ts->state = STATE_DO;
						break;
					case TELNET_DONT:
						ts->state = STATE_DONT;
						break;
					default:
						ts->state = STATE_NORMAL;
						break;
					}
				}
				break;

			case STATE_WILL:
				/* Reply with a DONT */
				sendopt(TELNET_DONT, c);
				ts->state = STATE_NORMAL;
				break;
			case STATE_WONT:
				/* Reply with a DONT */
				sendopt(TELNET_DONT, c);
				ts->state = STATE_NORMAL;
				break;
			case STATE_DO:
				/* Reply with a WONT */
				sendopt(TELNET_WONT, c);
				ts->state = STATE_NORMAL;
				break;
			case STATE_DONT:
				/* Reply with a WONT */
				sendopt(TELNET_WONT, c);
				ts->state = STATE_NORMAL;
				break;
			case STATE_NORMAL:
				if(c == TELNET_IAC) {
					ts->state = STATE_IAC;
				} else {
					console_insert(c);
				}
				break;
			}
		}
		pbuf_free(p);

#if USE_TCPBUF
		return tcpbuf_send(&telnetbuf, pcb);
#else
		return ERR_OK;
#endif
	}
	else
	{
		if (p)
			pbuf_free(p);
		telnet_close(pcb);

		return ERR_OK;
	}
}

static err_t server_poll(void *arg, struct tcp_pcb *pcb)
{
   LWIP_UNUSED_ARG(arg);
   LWIP_UNUSED_ARG(pcb);

   if (ts->max_idle != -1 && ++ts->idle >= ts->max_idle) { 
	   telnet_printf("\nYou have been idle for a while, goodbye\n");
	   telnet_close(ts->client);
	   return ERR_TIMEOUT;
   }

   return ERR_OK;
}

static void server_err(void *arg, err_t err)
{
	LWIP_UNUSED_ARG(arg);

	tcp_log_err(err);
}


static err_t tcp_data_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	if (pcb != ts->client) { 
	
		// close other than telnet tcp connection as soon as data have been sent
		
		tcp_sent(pcb, NULL);	// suppress callback
		tcp_output(pcb);	// flush output
		tcp_close(pcb);
	}
	
#if USE_TCPBUF
	return tcpbuf_send(&telnetbuf, pcb);
#else
	return ERR_OK;
#endif	
}

static err_t tcp_conn_accepted(void * arg, struct tcp_pcb * pcb, err_t err)
{
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);
	tcp_setprio(pcb, TCP_PRIO_MIN);
	tcp_accepted(pcb);
	tcp_arg(pcb, NULL);
	tcp_recv(pcb, server_recv);
	tcp_err(pcb, server_err);
	tcp_poll(pcb, server_poll, 4); //every two seconds of inactivity of the TCP connection
	tcp_sent(pcb, tcp_data_sent);
	
	static const char busy[] = "Sorry, but this telnet console is already in use by someone\n";
	if (ts->client) { /* Someone already connected */
		TCP_WRITE(pcb, busy, strlen(busy), 0);
		tcp_recv(pcb, NULL);
		tcp_poll(pcb, NULL, 4);
		tcp_err(pcb, NULL);		
		return ERR_OK; /* accept it till the error message is sent */
	}

	ts->client = pcb;

	console_printf("\ntelnet: Incoming connection, switching to telnet console...\n"); 
	ts->prev_printf = console_printf;
	console_printf = telnet_printf;
	microrl_set_echo(0);

	/* Send in a welcome message */
	console_printf("Welcome to %s!\n", env_get("hostname"));
	console_printf("Press enter to activate this console\n");

	ts->idle = 0;
	ts->max_idle = 60;

	const char *tmp = env_get("telnet-drop");
	if (tmp)
		ts->max_idle = atoi(tmp);

	return ERR_OK;
}

void telnet_start(int port)
{
	if (ts) {
		console_printf("telnet: server already started\n");
		return;
	}
	ts = os_malloc(sizeof(struct telnet_server));
	if (!ts) { 
		console_printf("telnet: out of memory!\n");
		return;
	}
	
	ts->server = tcp_new();
	if (!ts->server) {
		console_printf("telnet: Unable to allocate pcb\n");
		os_free(ts);
		ts = NULL;
		return;
	}
	
	tcp_bind(ts->server, IP_ADDR_ANY, 23);
	ts->server = tcp_listen(ts->server);
	if (!ts->server) {
		os_free(ts);
		ts = NULL;
		return;
	}
	tcp_accept(ts->server, tcp_conn_accepted);
	ts->client = NULL;
	ts->state  = STATE_NORMAL;	
	console_printf("telnet: server accepting connections on port %d\n", port);
}

void telnet_stop()
{

	if (!ts)
		return;
	telnet_close(ts->client);
	telnet_close(ts->server);
	os_free(ts);
	ts = NULL;
	console_printf("telnet: stopped!\n");
}


static int  do_telnet(int argc, const char* const* argv)
{
	int port = 23;
	const char *tmp = env_get("telnet-port");
	if (tmp)
		port = atoi(tmp);
	
	if (strcmp(argv[1], "start") == 0)
		telnet_start(port);

	if (strcmp(argv[1], "stop") == 0)
		telnet_stop();

	if (ts && strcmp(argv[1], "quit") == 0) {
		console_printf("telnet: See you!\n");
		telnet_close(ts->client);
	}
	
	return 0;
}


CONSOLE_CMD(telnet, 2, 2, 
	    do_telnet, NULL, NULL, 
	    "start/stop telnet server"
	    HELPSTR_NEWLINE "telnet start - start it"
	    HELPSTR_NEWLINE "telnet stop  - stop it"
	    HELPSTR_NEWLINE "telnet quit  - drop current client");


