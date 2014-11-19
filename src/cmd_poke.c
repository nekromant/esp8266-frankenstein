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


/*

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

*/


struct pokerface {
	struct espconn esp_conn;
	esp_tcp esptcp;
	char databuf[128];
	int datalen;
};


static void  connected(void *arg)
{
    struct pokerface *p = arg;
    console_printf("connected!\n");    
    espconn_sent(&p->esp_conn, p->databuf, p->datalen);

}


static void cleanup(struct pokerface *p)
{
    console_printf("cleaning up\n");
    espconn_disconnect(p);	
    os_free(p);
    console_lock(0);
}

static void  reconnect(struct pokerface *p, sint8 err)
{
	console_printf("err %d\n", err);
	cleanup(p);
}

static void datasent(void *p)
{
	console_printf("data sent\n");
	cleanup(p);
}


static int   do_poke(int argc, const char* argv[])
{
	struct pokerface *p = os_malloc(sizeof(struct pokerface));
	if (!p) {
		console_printf("Can't malloc enough to poke\n");
	}
	
	int port = skip_atoi(&argv[2]);
	p->esp_conn.type = ESPCONN_TCP;
	p->esp_conn.state = ESPCONN_NONE;
	p->esp_conn.proto.tcp = &p->esptcp;
	p->esp_conn.proto.tcp->local_port = espconn_port();
	p->esp_conn.proto.tcp->remote_port = port;
	uint32_t target = ipaddr_addr(argv[1]);
	strcpy(p->databuf, argv[3]);
	strcat(p->databuf, "\r\n");
	p->datalen = strlen(argv[3])+3;
	

	os_memcpy(p->esp_conn.proto.tcp->remote_ip, &target, 4);
	espconn_regist_connectcb(p, connected);	
	espconn_regist_reconcb(p, reconnect);
	espconn_regist_sentcb(p, datasent);
	espconn_connect(&p->esp_conn);
	console_lock(1);
}

static int  do_poke_interrupt()
{

}


CONSOLE_CMD(poke, 3, -1, 
	    do_poke, do_poke_interrupt, NULL, 
	    "Send data to a remote host. "
	    HELPSTR_NEWLINE "poke hostname port [data]"
);
