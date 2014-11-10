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


LOCAL struct espconn user_conn;

static int do_send(int argc, const char* argv[])
{
	
}

CONSOLE_CMD(send, -1, -1, 
	    do_send, NULL, NULL, 
	    "Send arbitary strings via TCP/IP"
	    HELPSTR_NEWLINE "send 192.168.0.1 8080 Hello world"
);



LOCAL void ICACHE_FLASH_ATTR
webserver_recv(void *arg, char *pusrdata, unsigned short length)
{
	console_printf("received %d bytes of data\n", length);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;

    console_printf("client %d.%d.%d.%d:%d err %d reconnect\n", pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
    		pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port, err);
}

LOCAL ICACHE_FLASH_ATTR
void webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;
    
    console_printf("client %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],pesp_conn->proto.tcp->remote_port);
}

LOCAL void ICACHE_FLASH_ATTR
webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
}

static int do_listen(int argc, const char* argv[])
{
	char* portstr = argv[1];
	uint32_t port = skip_strtoul(&portstr);
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;
	
	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, webserver_listen);
	espconn_accept(&esp_conn);
}

CONSOLE_CMD(listen, -1, -1, 
	    do_listen, NULL, NULL,
	    "Listen for incoming data on port. CTRL+C to terminate."
	    HELPSTR_NEWLINE "listen 8080"
);

/*

static int do_resolve(int argc, const char* argv[])
{
}

CONSOLE_CMD(resolve, 2, 2, do_resolve, NULL, 
	    "Get IP address via hostname."
	    HELPSTR_NEWLINE "resolve google.com"
);
*/
