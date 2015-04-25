#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/mem.h"
//#include "crypto/common.h"
#include "osapi.h"
#include "lwip/app/dhcpserver.h"

#ifndef LWIP_OPEN_SRC
#include "net80211/ieee80211_var.h"
#endif
#include "user_interface.h"

////////////////////////////////////////////////////////////////////////////////////
static const uint8_t xid[4] = {0xad, 0xde, 0x12, 0x23};
static u8_t old_xid[4] = {0};
static const uint8_t magic_cookie[4] = {99, 130, 83, 99};
static struct udp_pcb *pcb_dhcps;
static struct ip_addr broadcast_dhcps;
static struct ip_addr server_address;
static struct ip_addr client_address;//added
static struct ip_addr client_address_plus;
static struct dhcps_msg msg_dhcps;
struct dhcps_state s;

///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg��Ϣ�ṹ����������
 *
 * @param optptr -- DHCP msg��Ϣλ��
 * @param type -- Ҫ��ӵ�����option
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t*  add_msg_type(uint8_t *optptr, uint8_t type)
{

        *optptr++ = DHCP_OPTION_MSG_TYPE;
        *optptr++ = 1;
        *optptr++ = type;
        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg�ṹ������offerӦ������
 *
 * @param optptr -- DHCP msg��Ϣλ��
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t*  add_offer_options(uint8_t *optptr)
{
        struct ip_addr ipadd;

        ipadd.addr = *( (uint32_t *) &server_address);

#ifdef USE_CLASS_B_NET
        *optptr++ = DHCP_OPTION_SUBNET_MASK;
        *optptr++ = 4;  //length
        *optptr++ = 255;
        *optptr++ = 240;	
        *optptr++ = 0;
        *optptr++ = 0;
#else
        *optptr++ = DHCP_OPTION_SUBNET_MASK;
        *optptr++ = 4;  
        *optptr++ = 255;
        *optptr++ = 255;	
        *optptr++ = 255;
        *optptr++ = 0;
#endif

        *optptr++ = DHCP_OPTION_LEASE_TIME;
        *optptr++ = 4;  
        *optptr++ = 0x00;
        *optptr++ = 0x01;
        *optptr++ = 0x51;
        *optptr++ = 0x80; 	

        *optptr++ = DHCP_OPTION_SERVER_ID;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = ip4_addr2( &ipadd);
        *optptr++ = ip4_addr3( &ipadd);
        *optptr++ = ip4_addr4( &ipadd);

	    *optptr++ = DHCP_OPTION_ROUTER;
	    *optptr++ = 4;  
	    *optptr++ = ip4_addr1( &ipadd);
	    *optptr++ = ip4_addr2( &ipadd);
	    *optptr++ = ip4_addr3( &ipadd);
	    *optptr++ = ip4_addr4( &ipadd);

	    *optptr++ = DHCP_OPTION_DNS_SERVER;
	    *optptr++ = 12;  
#ifdef CLASS_B_NET
        *optptr++ = DHCP_OPTION_BROADCAST_ADDRESS;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = 255;
        *optptr++ = 255;
        *optptr++ = 255;
#else
        *optptr++ = DHCP_OPTION_BROADCAST_ADDRESS;
        *optptr++ = 4;  
        *optptr++ = ip4_addr1( &ipadd);
        *optptr++ = ip4_addr2( &ipadd);
        *optptr++ = ip4_addr3( &ipadd);
        *optptr++ = 255;
#endif

        *optptr++ = DHCP_OPTION_INTERFACE_MTU;
        *optptr++ = 2;  
#ifdef CLASS_B_NET
        *optptr++ = 0x05;	
        *optptr++ = 0xdc;
#else
        *optptr++ = 0x02;	
        *optptr++ = 0x40;
#endif

        *optptr++ = DHCP_OPTION_PERFORM_ROUTER_DISCOVERY;
        *optptr++ = 1;  
        *optptr++ = 0x00; 

        *optptr++ = 43;	
        *optptr++ = 6;	

        *optptr++ = 0x01;	
        *optptr++ = 4;  
        *optptr++ = 0x00;
        *optptr++ = 0x00;
        *optptr++ = 0x00;
        *optptr++ = 0x02; 	

        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ��DHCP msg�ṹ����ӽ����־����
 *
 * @param optptr -- DHCP msg��Ϣλ��
 *
 * @return uint8_t* ����DHCP msgƫ�Ƶ�ַ
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t*  add_end(uint8_t *optptr)
{

        *optptr++ = DHCP_OPTION_END;
        return optptr;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��DHCP msg�ṹ��
 *
 * @param -- m ָ�򴴽���DHCP msg�ṹ�����??
 */
///////////////////////////////////////////////////////////////////////////////////
static void  create_msg(struct dhcps_msg *m)
{
        struct ip_addr client;

        client.addr = *( (uint32_t *) &client_address);

        m->op = DHCP_REPLY;
        m->htype = DHCP_HTYPE_ETHERNET;
        m->hlen = 6;  
        m->hops = 0;
        os_memcpy((char *) xid, (char *) m->xid, sizeof(m->xid));
        m->secs = 0;
        m->flags = htons(BOOTP_BROADCAST); 

        os_memcpy((char *) m->yiaddr, (char *) &client.addr, sizeof(m->yiaddr));

        os_memset((char *) m->ciaddr, 0, sizeof(m->ciaddr));
        os_memset((char *) m->siaddr, 0, sizeof(m->siaddr));
        os_memset((char *) m->giaddr, 0, sizeof(m->giaddr));
        os_memset((char *) m->sname, 0, sizeof(m->sname));
        os_memset((char *) m->file, 0, sizeof(m->file));

        os_memset((char *) m->options, 0, sizeof(m->options));
        os_memcpy((char *) m->options, (char *) magic_cookie, sizeof(magic_cookie));
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��OFFER
 *
 * @param -- m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void  send_offer(struct dhcps_msg *m)
{
        uint8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendOffer_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPOFFER);
        end = add_offer_options(end);
        end = add_end(end);

	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		os_printf("udhcp: send_offer>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	       
#if DHCPS_DEBUG
	        os_printf("dhcps: send_offer>>pbuf_alloc succeed\n");
	        os_printf("dhcps: send_offer>>p->tot_len = %d\n", p->tot_len);
	        os_printf("dhcps: send_offer>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG
					os_printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						os_printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	        
#if DHCPS_DEBUG
	        os_printf("dhcps: send_offer>>pbuf_alloc failed\n");
#endif
	        return;
	    }
        SendOffer_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        os_printf("dhcps: send_offer>>udp_sendto result %x\n",SendOffer_err_t);
#endif
	    if(p->ref != 0){	
#if DHCPS_DEBUG
	        os_printf("udhcp: send_offer>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��NAK��Ϣ
 *
 * @param m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void  send_nak(struct dhcps_msg *m)
{

    	u8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendNak_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPNAK);
        end = add_end(end);

	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		os_printf("udhcp: send_nak>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	        
#if DHCPS_DEBUG
	        os_printf("dhcps: send_nak>>pbuf_alloc succeed\n");
	        os_printf("dhcps: send_nak>>p->tot_len = %d\n", p->tot_len);
	        os_printf("dhcps: send_nak>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG					
					os_printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						os_printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	        
#if DHCPS_DEBUG
	        os_printf("dhcps: send_nak>>pbuf_alloc failed\n");
#endif
	        return;
    	}
        SendNak_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        os_printf("dhcps: send_nak>>udp_sendto result %x\n",SendNak_err_t);
#endif
 	    if(p->ref != 0){
#if DHCPS_DEBUG			
	        os_printf("udhcp: send_nak>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����һ��ACK��DHCP�ͻ���
 *
 * @param m ָ����Ҫ���͵�DHCP msg����
 */
///////////////////////////////////////////////////////////////////////////////////
static void  send_ack(struct dhcps_msg *m)
{

		u8_t *end;
	    struct pbuf *p, *q;
	    u8_t *data;
	    u16_t cnt=0;
	    u16_t i;
		err_t SendAck_err_t;
        create_msg(m);

        end = add_msg_type(&m->options[4], DHCPACK);
        end = add_offer_options(end);
        end = add_end(end);
	    
	    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcps_msg), PBUF_RAM);
#if DHCPS_DEBUG
		os_printf("udhcp: send_ack>>p->ref = %d\n", p->ref);
#endif
	    if(p != NULL){
	        
#if DHCPS_DEBUG
	        os_printf("dhcps: send_ack>>pbuf_alloc succeed\n");
	        os_printf("dhcps: send_ack>>p->tot_len = %d\n", p->tot_len);
	        os_printf("dhcps: send_ack>>p->len = %d\n", p->len);
#endif
	        q = p;
	        while(q != NULL){
	            data = (u8_t *)q->payload;
	            for(i=0; i<q->len; i++)
	            {
	                data[i] = ((u8_t *) m)[cnt++];
#if DHCPS_DEBUG					
					os_printf("%02x ",data[i]);
					if((i+1)%16 == 0){
						os_printf("\n");
					}
#endif
	            }

	            q = q->next;
	        }
	    }else{
	    
#if DHCPS_DEBUG
	        os_printf("dhcps: send_ack>>pbuf_alloc failed\n");
#endif
	        return;
	    }
        SendAck_err_t = udp_sendto( pcb_dhcps, p, &broadcast_dhcps, DHCPS_CLIENT_PORT );
#if DHCPS_DEBUG
	        os_printf("dhcps: send_ack>>udp_sendto result %x\n",SendAck_err_t);
#endif
	    
	    if(p->ref != 0){
#if DHCPS_DEBUG
	        os_printf("udhcp: send_ack>>free pbuf\n");
#endif
	        pbuf_free(p);
	    }
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * ����DHCP�ͻ��˷�����DHCP����������Ϣ�����Բ�ͬ��DHCP��������������Ӧ��Ӧ��
 *
 * @param optptr DHCP msg�е���������
 * @param len ��������Ĵ��?(byte)
 *
 * @return uint8_t ���ش�����DHCP Server״ֵ̬
 */
///////////////////////////////////////////////////////////////////////////////////
static uint8_t  parse_options(uint8_t *optptr, sint16_t len)
{
        struct ip_addr client;
    	bool is_dhcp_parse_end = false;

        client.addr = *( (uint32_t *) &client_address);// Ҫ�����DHCP�ͻ��˵�IP

        u8_t *end = optptr + len;
        u16_t type = 0;

        s.state = DHCPS_STATE_IDLE;

        while (optptr < end) {
#if DHCPS_DEBUG
        	os_printf("dhcps: (sint16_t)*optptr = %d\n", (sint16_t)*optptr);
#endif
        	switch ((sint16_t) *optptr) {

                case DHCP_OPTION_MSG_TYPE:	//53
                        type = *(optptr + 2);
                        break;

                case DHCP_OPTION_REQ_IPADDR://50
                        if( os_memcmp( (char *) &client.addr, (char *) optptr+2,4)==0 ) {
#if DHCPS_DEBUG
                    		os_printf("dhcps: DHCP_OPTION_REQ_IPADDR = 0 ok\n");
#endif
                            s.state = DHCPS_STATE_ACK;
                        }else {
#if DHCPS_DEBUG
                    		os_printf("dhcps: DHCP_OPTION_REQ_IPADDR != 0 err\n");
#endif
                            s.state = DHCPS_STATE_NAK;
                        }
                        break;
                case DHCP_OPTION_END:
			            {
			                is_dhcp_parse_end = true;
			            }
                        break;
            }

		    if(is_dhcp_parse_end){
		            break;
		    }

            optptr += optptr[1] + 2;
        }

        switch (type){
        
        	case DHCPDISCOVER://1
                s.state = DHCPS_STATE_OFFER;
#if DHCPS_DEBUG
            	os_printf("dhcps: DHCPD_STATE_OFFER\n");
#endif
                break;

        	case DHCPREQUEST://3
                if ( !(s.state == DHCPS_STATE_ACK || s.state == DHCPS_STATE_NAK) ) {
                        s.state = DHCPS_STATE_NAK;
#if DHCPS_DEBUG
                		os_printf("dhcps: DHCPD_STATE_NAK\n");
#endif
                }
                break;

			case DHCPDECLINE://4
                s.state = DHCPS_STATE_IDLE;
#if DHCPS_DEBUG
            	os_printf("dhcps: DHCPD_STATE_IDLE\n");
#endif
                break;

        	case DHCPRELEASE://7
                s.state = DHCPS_STATE_IDLE;
#if DHCPS_DEBUG
            	os_printf("dhcps: DHCPD_STATE_IDLE\n");
#endif
                break;
        }
#if DHCPS_DEBUG
    	os_printf("dhcps: return s.state = %d\n", s.state);
#endif
        return s.state;
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
static sint16_t  parse_msg(struct dhcps_msg *m, u16_t len)
{
		if(os_memcmp((char *)m->options,
              (char *)magic_cookie,
              sizeof(magic_cookie)) == 0){
#if DHCPS_DEBUG
        	os_printf("dhcps: len = %d\n", len);
#endif
	        /*
         	 * ��¼��ǰ��xid���ﴦ���??
         	 * �˺�ΪDHCP�ͻ����������û�ͳһ��ȡIPʱ��
         	*/
	        if((old_xid[0] == 0) &&
	           (old_xid[1] == 0) &&
	           (old_xid[2] == 0) &&
	           (old_xid[3] == 0)){
	            /* 
	             * old_xidδ��¼�κ����??
	             * �϶��ǵ�һ��ʹ��
	            */
	            os_memcpy((char *)old_xid, (char *)m->xid, sizeof(m->xid));
	        }else{
	            /*
	             * ���δ����DHCP msg��Я���xid���ϴμ�¼�Ĳ�ͬ��
	             * �϶�Ϊ��ͬ��DHCP�ͻ��˷��ͣ���ʱ����Ҫ����Ŀͻ���IP
	             * ���� 192.168.4.100(0x6404A8C0) <--> 192.168.4.200(0xC804A8C0)
	             * 
	            */
	            if(os_memcmp((char *)old_xid, (char *)m->xid, sizeof(m->xid)) != 0){
	                /*
                 	 * ��¼���ε�xid�ţ�ͬʱ�����IP����
                 	*/
	                struct ip_addr addr_tmp;    
	                os_memcpy((char *)old_xid, (char *)m->xid, sizeof(m->xid));

	                {
						struct station_info *station = wifi_softap_get_station_info();
						struct station_info *next_station;
						struct station_info *back_station = station;
						uint8 find = 0;

						while(station) {
							if (os_memcmp(station->bssid, m->chaddr, 6) == 0) {
								find = 1;
								break;
							}
							next_station = STAILQ_NEXT(station, next);
							station = next_station;
						}

						if (find == 0) {
							station = back_station;
							while(station) {
								if (station->ip.addr == client_address_plus.addr) {
									addr_tmp.addr =  htonl(client_address_plus.addr);
									addr_tmp.addr++;
									client_address_plus.addr = htonl(addr_tmp.addr);
									station = back_station;
									continue;
								}
								next_station = STAILQ_NEXT(station, next);
								station = next_station;
							}

							if (wifi_softap_set_station_info(m->chaddr, &client_address_plus) == false) {
								return 0;
							}
							client_address.addr = client_address_plus.addr;
							addr_tmp.addr =  htonl(client_address_plus.addr);
							addr_tmp.addr++;
							client_address_plus.addr = htonl(addr_tmp.addr);

							if(ip4_addr4(&client_address_plus) > 0xC8){
								ip4_addr4(&client_address_plus) = 0x64;
							}
						} else {
							client_address.addr = station->ip.addr;
						}

						wifi_softap_free_station_info();
					}

#if DHCPS_DEBUG
	                os_printf("dhcps: xid changed\n");
	                os_printf("dhcps: client_address.addr = %x\n", client_address.addr);
#endif
	               
	            }
	            
	        }
                    
	        return parse_options(&m->options[4], len);
	    }
        return 0;
}
///////////////////////////////////////////////////////////////////////////////////
/*
 * DHCP ��������ݰ���մ���ص�����˺�����LWIP UDPģ������ʱ������
 * ��Ҫ����udp_recv()������LWIP����ע��.
 *
 * @param arg
 * @param pcb ���յ�UDP��Ŀ��ƿ�??
 * @param p ���յ���UDP�е��������??
 * @param addr ���ʹ�UDP���Դ�����IP��ַ
 * @param port ���ʹ�UDP���Դ�����UDPͨ���˿ں�
 */
///////////////////////////////////////////////////////////////////////////////////
static void  handle_dhcp(void *arg, 
									struct udp_pcb *pcb, 
									struct pbuf *p, 
									struct ip_addr *addr, 
									uint16_t port)
{
		
		sint16_t tlen;
        u16_t i;
	    u16_t dhcps_msg_cnt=0;
	    u8_t *p_dhcps_msg = (u8_t *)&msg_dhcps;
	    u8_t *data;

#if DHCPS_DEBUG
    	os_printf("dhcps: handle_dhcp-> receive a packet\n");
#endif
	    if (p==NULL) return;

		tlen = p->tot_len;
	    data = p->payload;

#if DHCPS_DEBUG
	    os_printf("dhcps: handle_dhcp-> p->tot_len = %d\n", tlen);
	    os_printf("dhcps: handle_dhcp-> p->len = %d\n", p->len);
#endif		

	    os_memset(&msg_dhcps, 0, sizeof(dhcps_msg));
	    for(i=0; i<p->len; i++){
	        p_dhcps_msg[dhcps_msg_cnt++] = data[i];
#if DHCPS_DEBUG					
			os_printf("%02x ",data[i]);
			if((i+1)%16 == 0){
				os_printf("\n");
			}
#endif
	    }
		
		if(p->next != NULL) {
#if DHCPS_DEBUG
	        os_printf("dhcps: handle_dhcp-> p->next != NULL\n");
	        os_printf("dhcps: handle_dhcp-> p->next->tot_len = %d\n",p->next->tot_len);
	        os_printf("dhcps: handle_dhcp-> p->next->len = %d\n",p->next->len);
#endif
			
	        data = p->next->payload;
	        for(i=0; i<p->next->len; i++){
	            p_dhcps_msg[dhcps_msg_cnt++] = data[i];
#if DHCPS_DEBUG					
				os_printf("%02x ",data[i]);
				if((i+1)%16 == 0){
					os_printf("\n");
				}
#endif
			}
		}

		/*
	     * DHCP �ͻ���������Ϣ����
	    */
#if DHCPS_DEBUG
    	os_printf("dhcps: handle_dhcp-> parse_msg(p)\n");
#endif
		
        switch(parse_msg(&msg_dhcps, tlen - 240)) {

	        case DHCPS_STATE_OFFER://1
#if DHCPS_DEBUG            
            	 os_printf("dhcps: handle_dhcp-> DHCPD_STATE_OFFER\n");
#endif			
	             send_offer(&msg_dhcps);
	             break;
	        case DHCPS_STATE_ACK://3
#if DHCPS_DEBUG
            	 os_printf("dhcps: handle_dhcp-> DHCPD_STATE_ACK\n");
#endif			
	             send_ack(&msg_dhcps);
	             break;
	        case DHCPS_STATE_NAK://4
#if DHCPS_DEBUG            
            	 os_printf("dhcps: handle_dhcp-> DHCPD_STATE_NAK\n");
#endif
	             send_nak(&msg_dhcps);
	             break;
			default :
				 break;
        }
#if DHCPS_DEBUG
    	os_printf("dhcps: handle_dhcp-> pbuf_free(p)\n");
#endif
        pbuf_free(p);
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void  dhcps_start(struct ip_info *info)
{
		os_memset(&msg_dhcps, 0, sizeof(dhcps_msg));
		pcb_dhcps = udp_new();
		if(pcb_dhcps==NULL){
			os_printf("dhcps_start(): could not obtain pcb\n");
		}

		IP4_ADDR(&broadcast_dhcps,255,255,255,255);

		server_address = info->ip;
		IP4_ADDR(&client_address_plus,ip4_addr1(&info->ip),
				ip4_addr2(&info->ip),
				ip4_addr3(&info->ip),100);

        udp_bind(pcb_dhcps, IP_ADDR_ANY, DHCPS_SERVER_PORT );
        udp_recv(pcb_dhcps, handle_dhcp, NULL);
#if DHCPS_DEBUG
		os_printf("dhcps:dhcps_start->udp_recv function Set a receive callback handle_dhcp for UDP_PCB pcb_dhcps\n");
#endif
		
}
