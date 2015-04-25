/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: espconn_udp.c
 *
 * Description: udp proto interface
 *
 * Modification history:
 *     2014/3/31, v1.0 create this file.
*******************************************************************************/

#include "ets_sys.h"
#include "os_type.h"
//#include "os.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"

#include "lwip/app/espconn_udp.h"

extern espconn_msg *plink_active;

static void  espconn_data_sentcb(struct espconn *pespconn)
{
    if (pespconn == NULL) {
        return;
    }

    if (pespconn->sent_callback != NULL) {
        pespconn->sent_callback(pespconn);
    }
}

static void  espconn_data_sent(void *arg)
{
    espconn_msg *psent = arg;

    if (psent == NULL) {
        return;
    }

    if (psent->pcommon.cntr == 0) {
        psent->pespconn->state = ESPCONN_CONNECT;
        sys_timeout(TCP_FAST_INTERVAL, espconn_data_sentcb, psent->pespconn);
    } else {
        espconn_udp_sent(arg, psent->pcommon.ptrbuf, psent->pcommon.cntr);
    }
}

/******************************************************************************
 * FunctionName : espconn_udp_sent
 * Description  : sent data for client or server
 * Parameters   : void *arg -- client or server to send
 * 				  uint8* psent -- Data to send
 *                uint16 length -- Length of data to send
 * Returns      : none
*******************************************************************************/
void 
espconn_udp_sent(void *arg, uint8 *psent, uint16 length)
{
    espconn_msg *pudp_sent = arg;
    struct udp_pcb *upcb = pudp_sent->pcommon.pcb;
    struct pbuf *p, *q;
    u8_t *data = NULL;
    u16_t cnt = 0;
    u16_t datalen = 0;
    u16_t i = 0;
    err_t err;
    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %d %p\n", __LINE__, length, upcb));

    if (pudp_sent == NULL || upcb == NULL || psent == NULL || length == 0) {
        return;
    }

    if (1024 < length) {
        datalen = 1024;
    } else {
        datalen = length;
    }

    p = pbuf_alloc(PBUF_TRANSPORT, datalen, PBUF_RAM);
    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %p\n", __LINE__, p));

    if (p != NULL) {
        q = p;

        while (q != NULL) {
            data = (u8_t *)q->payload;
            LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %p\n", __LINE__, data));

            for (i = 0; i < q->len; i++) {
                data[i] = ((u8_t *) psent)[cnt++];
            }

            q = q->next;
        }
    } else {
        return;
    }

    upcb->remote_port = pudp_sent->pespconn->proto.udp->remote_port;
    IP4_ADDR(&upcb->remote_ip, pudp_sent->pespconn->proto.udp->remote_ip[0],
    		pudp_sent->pespconn->proto.udp->remote_ip[1],
    		pudp_sent->pespconn->proto.udp->remote_ip[2],
    		pudp_sent->pespconn->proto.udp->remote_ip[3]);

    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %x %d\n", __LINE__, upcb->remote_ip, upcb->remote_port));
    err = udp_send(upcb, p);
    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %d\n", __LINE__, err));

    if (p->ref != 0) {
        LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_sent %d %p\n", __LINE__, p));
        pbuf_free(p);
        pudp_sent->pcommon.ptrbuf = psent + datalen;
        pudp_sent->pcommon.cntr = length - datalen;
        espconn_data_sent(pudp_sent);
    }
}

/******************************************************************************
 * FunctionName : espconn_udp_client_recv
 * Description  : This callback will be called when receiving a datagram.
 * Parameters   : arg -- user supplied argument
 *                upcb -- the udp_pcb which received data
 *                p -- the packet buffer that was received
 *                addr -- the remote IP address from which the packet was received
 *                port -- the remote port from which the packet was received
 * Returns      : none
*******************************************************************************/
#if 0
static void 
espconn_udp_client_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                        struct ip_addr *addr, u16_t port)
{
    struct espconn *pespconn = NULL;
    struct pbuf *q = NULL;
    u8_t *pdata = NULL;
    u16_t length = 0;
    pespconn = arg;
    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_client_recv %d %p\n", __LINE__, upcb));

    upcb->remote_port = port;
    upcb->remote_ip = *addr;

    os_memcpy(pespconn->proto.udp->ipaddr, addr, 4);

    if (p != NULL) {
        q = p;

        while (q != NULL) {
            pdata = (char *)os_zalloc(q ->len + 1);
            length = pbuf_copy_partial(q, pdata, q ->len, 0);

            if (length != 0) {
                LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_client_recv %d %d\n", __LINE__, length));
                pespconn->esp_pcb = upcb;

                if (pespconn->recv_callback != NULL) {
                    pespconn->recv_callback(arg, pdata, length);
                }
            }

            q = q ->next;
            os_free(pdata);
        }

        pbuf_free(p);
    } else {
        return;
    }
}
#endif

/******************************************************************************
 * FunctionName : espconn_udp_server_recv
 * Description  : This callback will be called when receiving a datagram.
 * Parameters   : arg -- user supplied argument
 *                upcb -- the udp_pcb which received data
 *                p -- the packet buffer that was received
 *                addr -- the remote IP address from which the packet was received
 *                port -- the remote port from which the packet was received
 * Returns      : none
*******************************************************************************/
static void 
espconn_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                 struct ip_addr *addr, u16_t port)
{
    espconn_msg *precv = arg;
    struct pbuf *q = NULL;
    u8_t *pdata = NULL;
    u16_t length = 0;

    LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_server_recv %d %p\n", __LINE__, upcb));

    upcb->remote_port = port;
    upcb->remote_ip = *addr;

    precv->pcommon.remote_ip[0] = ip4_addr1_16(&upcb->remote_ip);
    precv->pcommon.remote_ip[1] = ip4_addr2_16(&upcb->remote_ip);
    precv->pcommon.remote_ip[2] = ip4_addr3_16(&upcb->remote_ip);
    precv->pcommon.remote_ip[3] = ip4_addr4_16(&upcb->remote_ip);
    os_memcpy(precv->pespconn->proto.udp->remote_ip, precv->pcommon.remote_ip, 4);
    precv->pespconn->proto.udp->remote_port = port;
    precv->pcommon.remote_port = port;
    precv->pcommon.pcb = upcb;

    if (p != NULL) {
        q = p;

        while (q != NULL) {
            pdata = (u8_t *)os_zalloc(q ->len + 1);
            length = pbuf_copy_partial(q, pdata, q ->len, 0);

            LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("espconn_udp_server_recv %d %x\n", __LINE__, length));
            precv->pcommon.pcb = upcb;

            if (length != 0) {
                if (precv->pespconn->recv_callback != NULL) {
                    precv->pespconn->recv_callback(precv->pespconn, pdata, length);
                }
            }

            q = q->next;
            os_free(pdata);
        }

        pbuf_free(p);
    } else {
        return;
    }
}

/******************************************************************************
 * FunctionName : espconn_udp_disconnect
 * Description  : A new incoming connection has been disconnected.
 * Parameters   : espconn -- the espconn used to disconnect with host
 * Returns      : none
*******************************************************************************/
void  espconn_udp_disconnect(espconn_msg *pdiscon)
{
    if (pdiscon == NULL) {
        return;
    }

    struct udp_pcb *upcb = pdiscon->pcommon.pcb;

    udp_disconnect(upcb);

    udp_remove(upcb);

    if (pdiscon->preverse != NULL) {
        espconn_list_delete(&plink_active, pdiscon);
    } else {
        espconn_list_delete(&plink_active, pdiscon);
    }

    os_free(pdiscon);
    pdiscon = NULL;
}

/******************************************************************************
 * FunctionName : espconn_udp_client
 * Description  : Initialize the client: set up a PCB and bind it to the port
 * Parameters   : pespconn -- the espconn used to build client
 * Returns      : none
*******************************************************************************/
sint8 
espconn_udp_client(struct espconn *pespconn)
{
    struct udp_pcb *upcb;
    struct ip_addr ipaddr;
    espconn_msg *pclient = NULL;
    pclient = (espconn_msg *)os_zalloc(sizeof(espconn_msg));

    if (pclient == NULL) {
        return ESPCONN_MEM;
    }

    IP4_ADDR(&ipaddr, pespconn->proto.udp->remote_ip[0],
             pespconn->proto.udp->remote_ip[1],
             pespconn->proto.udp->remote_ip[2],
             pespconn->proto.udp->remote_ip[3]);

    upcb = udp_new();

    if (upcb == NULL) {
        os_free(pclient);
        pclient = NULL;
        return ESPCONN_MEM;
    } else {
        pclient->pcommon.pcb = upcb;
        pclient->preverse = NULL;
        pclient->pespconn = pespconn;
        espconn_list_creat(&plink_active, pclient);
        udp_bind(upcb, IP_ADDR_ANY, pclient->pespconn->proto.udp->local_port);
        udp_recv(upcb, espconn_udp_recv, (void *)pclient);
        pclient->pcommon.err = udp_connect(upcb, &ipaddr, pclient->pespconn->proto.udp->remote_port);
        return pclient->pcommon.err;
    }
}

/******************************************************************************
 * FunctionName : espconn_udp_server
 * Description  : Initialize the server: set up a PCB and bind it to the port
 * Parameters   : pespconn -- the espconn used to build server
 * Returns      : none
*******************************************************************************/
sint8 
espconn_udp_server(struct espconn *pespconn)
{
    struct udp_pcb *upcb = NULL;
    espconn_msg *pserver = NULL;
    upcb = udp_new();

    if (upcb == NULL) {
        return ESPCONN_MEM;
    } else {
        pserver = (espconn_msg *)os_zalloc(sizeof(espconn_msg));

        if (pserver == NULL) {
            udp_remove(upcb);
            return ESPCONN_MEM;
        }

        pserver->pcommon.pcb = upcb;
        pserver->preverse = pespconn;
        pserver->pespconn = pespconn;
        espconn_list_creat(&plink_active, pserver);
        udp_bind(upcb, IP_ADDR_ANY, pserver->pespconn->proto.udp->local_port);
        udp_recv(upcb, espconn_udp_recv, (void *)pserver);
        return ESPCONN_OK;
    }
}

/******************************************************************************
 * FunctionName : espconn_igmp_leave
 * Description  : leave a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 
espconn_igmp_leave(ip_addr_t *host_ip, ip_addr_t *multicast_ip)
{
    if (igmp_leavegroup(host_ip, multicast_ip) != ERR_OK) {
        LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("udp_leave_multigrup failed!\n"));
        return -1;
    };

    return ESPCONN_OK;
}

/******************************************************************************
 * FunctionName : espconn_igmp_join
 * Description  : join a multicast group
 * Parameters   : host_ip -- the ip address of udp server
 * 				  multicast_ip -- multicast ip given by user
 * Returns      : none
*******************************************************************************/
sint8 
espconn_igmp_join(ip_addr_t *host_ip, ip_addr_t *multicast_ip)
{
    if (igmp_joingroup(host_ip, multicast_ip) != ERR_OK) {
        LWIP_DEBUGF(ESPCONN_UDP_DEBUG, ("udp_join_multigrup failed!\n"));
        return -1;
    };

    /* join to any IP address at the port  */
    return ESPCONN_OK;
}
