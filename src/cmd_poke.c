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

static void  connected(void *arg)
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

static int   do_poke(int argc, const char* argv[])
{
	int port = skip_atoi(&argv[1]);

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->remote_port = port;
	espconn_regist_connectcb(&esp_conn, connected);
	espconn_create(&ptrespconn);

	console_lock(1);
}

static int  do_poke_interrupt()
{
	console_printf("BUG: How on earth to properly stop listening???\n");
	espconn_disconnect(&esp_conn);
	esp_conn.state = ESPCONN_NONE;
//	os_free(linebuffer);
	console_lock(0);
}


CONSOLE_CMD(poke, -1, -1, 
	    do_poke, do_poke_interrupt, NULL, 
	    "Send data to a remote host. "
	    HELPSTR_NEWLINE "poke hostname port [data]"
);

/*
CONSOLE_CMD(pokeback, -1, -1, 
	    do_pokeback, do_poke_interrupt, NULL, 
	    "Send data to a remote host and dump response"
	    HELPSTR_NEWLINE "poke hostname port [data]"
);

*/
