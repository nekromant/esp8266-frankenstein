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

void ping_done(struct pbuf *p, struct icmp_echo_hdr *iecho)
{
	console_printf("Ping done!\n");
}


static  void ICACHE_FLASH_ATTR
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

static void ICACHE_FLASH_ATTR webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;
    console_printf("reconnect  | %d.%d.%d.%d:%d, error %d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port, err);
}

static void ICACHE_FLASH_ATTR webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;

    console_printf("disconnect | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR webserver_listen(void *arg)
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

static int ICACHE_FLASH_ATTR  do_listen(int argc, const char* argv[])
{
	int port = skip_atoi(&argv[1]);
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
}

static int ICACHE_FLASH_ATTR do_listen_interrupt()
{
	console_printf("BUG: How on earth to properly stop listening???\n");
	espconn_disconnect(&esp_conn);
	esp_conn.state = ESPCONN_NONE;
//	os_free(linebuffer);
	console_lock(0);
}

CONSOLE_CMD(listen, -1, -1, 
	    do_listen, do_listen_interrupt, NULL, 
	    "Listen for incoming data ona port"
	    HELPSTR_NEWLINE "listen 8080"
);
