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

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>

#define LINEBUFFER_SIZE 1024
static char* linebuffer;
static int lineptr; 
static int disconn = 1; 


static  void 
webserver_recv(void *arg, char *pusrdata, unsigned short length)
{
	while (length--) {
		if ((*pusrdata == 13) || (*pusrdata == 10))
		{
			linebuffer[lineptr++]=0x0;
			console_printf("receive    | %s\n", linebuffer);
			lineptr = 0;
			pusrdata++;
			if (disconn) {
				espconn_disconnect(arg);
				return;
			}
			continue;
		} else  if (lineptr == LINEBUFFER_SIZE) {
			linebuffer[LINEBUFFER_SIZE - 1]=0x0;
			console_printf("receive   | %s\n", linebuffer);
			lineptr = 0;
			pusrdata++;
			if (disconn) {
				espconn_disconnect(arg);
				return;
			}
			continue;
		}
		linebuffer[lineptr++] = *pusrdata++;
	}

	
}

static void  webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;
    console_printf("reconnect  | %d.%d.%d.%d:%d, error %d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port, err);
}

static void  webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;

    console_printf("disconnect | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);
}

static void  webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    console_printf("connect    | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);

}

static struct espconn esp_conn;
static esp_tcp esptcp;

static int   do_listen(int argc, const char* const* argv)
{
	int port = atoi(argv[1]);
	console_printf("Listening (TCP) on port %d\n", port);
	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, webserver_listen);
	espconn_accept(&esp_conn);
	linebuffer = os_malloc(LINEBUFFER_SIZE);
	lineptr = 0;
	console_lock(1);
	return 0;
}

static void do_listen_interrupt(void)
{
}

CONSOLE_CMD(listen, 2, 2, 
	    do_listen, do_listen_interrupt, NULL, 
	    "Listen for incoming data ona port"
	    HELPSTR_NEWLINE "listen 8080"
);
