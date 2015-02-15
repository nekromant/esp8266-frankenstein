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


struct pokerface {
	struct espconn esp_conn;
	esp_tcp esptcp;
	char databuf[128];
	int datalen;
	/*volatile*/ os_timer_t conn_checker;
};

static void conn_checker_handler(void *arg)
{
	struct pokerface *p = arg;
	/* Lazy gc */
	os_free(p);
	console_printf("free!\n");
	
}

static void  connected(void *arg)
{
	struct pokerface *p = arg;
	espconn_sent(&p->esp_conn, (uint8*)p->databuf, p->datalen);
}

static void  disconnected(void *arg)
{
	console_printf("OK\n");
	console_lock(0);
	struct pokerface *p = arg;
	os_timer_arm(&p->conn_checker, 50, 0);
	
}

static void  reconnect(void *arg, sint8 err)
{
	console_printf("Error %d\n", err);
	struct pokerface *p = arg;
	espconn_disconnect(&p->esp_conn);
	console_lock(0);
}

static void datasent(void *arg)
{
	struct pokerface *p = arg;
	os_timer_disarm(&p->conn_checker);
	os_timer_setfn(&p->conn_checker, (os_timer_func_t *)conn_checker_handler, p);
	espconn_disconnect(&p->esp_conn);
}


static int   do_send(int argc, const char* const* argv)
{
	struct pokerface *p = os_zalloc(sizeof(struct pokerface));
	if (!p) {
		console_printf("Can't malloc enough to send\n");
		return -1;
	}
	
	int port = atoi(argv[2]);
	p->esp_conn.type = ESPCONN_TCP;
	p->esp_conn.state = ESPCONN_NONE;
	p->esp_conn.proto.tcp = &p->esptcp;
	p->esp_conn.proto.tcp->local_port = espconn_port();
	p->esp_conn.proto.tcp->remote_port = port;
	uint32_t target = ipaddr_addr(argv[1]);
	
	int i; 
	p->databuf[0] = 0;
	for (i=3; i<argc; i++) { 
		strcat(p->databuf, argv[i]);
		strcat(p->databuf, " ");
	}
	p->datalen = strlen(p->databuf);
	
	os_memcpy(p->esp_conn.proto.tcp->remote_ip, &target, 4);
	espconn_regist_connectcb(&p->esp_conn, connected);	
	espconn_regist_reconcb(&p->esp_conn, reconnect);
	espconn_regist_disconcb(&p->esp_conn, disconnected);
	espconn_regist_sentcb(&p->esp_conn, datasent);
	espconn_connect(&p->esp_conn);
	console_lock(1);

	return 0;
}

static void do_send_interrupt(void)
{

}


CONSOLE_CMD(send, 4, -1, 
	    do_send, do_send_interrupt, NULL, 
	    "Send data to a remote host. "
	    HELPSTR_NEWLINE "send hostname port [data]"
);
