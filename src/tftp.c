#include <string.h>
#include <stdio.h>
#include <generic/macros.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>

#include <lib/tftp.h>

/*
  Spec: 
  1     Read request (RRQ)
  2     Write request (WRQ)1
  3     Data (DATA)
  4     Acknowledgment (ACK)
  5     Error (ERROR)
*/

#define OP_RRQ  1
#define OP_WRQ  2
#define OP_DATA 3
#define OP_ACK  4
#define OP_ERR  5


/* TODO: Move endianness swap to kconfig */

#define SWAP16(num) ((num>>8) | (num<<8))


static err_t tftp_send_message(struct udp_pcb *upcb, struct ip_addr *to_ip, int to_port, char *buf, int buflen) {

	err_t err;
	struct pbuf *pkt_buf;
  
	pkt_buf = pbuf_alloc(PBUF_TRANSPORT, buflen, PBUF_POOL);
	if (!pkt_buf) {
		return ERR_MEM;
	}

	memcpy(pkt_buf->payload, buf, buflen);
	err = udp_sendto(upcb, pkt_buf, to_ip, to_port);

	pbuf_free(pkt_buf);
	return err;
}

void tftp_request(struct tftp_server *ts, char* host, char* dir, char *fname)
{
	char tftp_buffer[512+4];
	uint32_t length;
        tftp_buffer[0] = 0;                     /* op */
        tftp_buffer[1] = OP_RRQ;   
        length = sprintf((char *) &tftp_buffer[2], "%s", fname) + 2;
        tftp_buffer[length] = 0; 
	length++;
        length += sprintf((char*)&tftp_buffer[length], "%s", "octet");
        tftp_buffer[length] = 0; 
	length++;
	tftp_send_message(ts->out, &ts->addr, ts->port, tftp_buffer, length);
	ts->numblock = 1;
}


struct tftp_packet {
	uint16_t op;
	uint16_t block;
	char data[];
} __attribute__((packed)); 


static void tftp_ack(struct tftp_server *ts, uint16_t pck, uint16_t port) 
{
	struct tftp_packet ack; 
	ack.op = SWAP16(OP_ACK);
	ack.block = SWAP16(pck);
	tftp_send_message(ts->out, &ts->addr, port, &ack, sizeof(ack));
}


static void udp_tftp_recv(void * arg, struct udp_pcb * upcb,
                                         struct pbuf * p,
                                         struct ip_addr * addr,
                                         u16_t port)
{
	struct tftp_server *ts = arg;
	if (!p)
		return;

	char tmp[1024];
	pbuf_copy_partial(p, tmp, p->tot_len, 0);

	struct tftp_packet *pck = tmp;

	pck->op = SWAP16(pck->op);
	pck->block = SWAP16(pck->block);

	if (pck->op == OP_ERR) { 
		if (ts->err)
			ts->err(ts, pck->block, pck->data);
		
	} else if (pck->op == OP_DATA) { 
		if ((pck->block == ts->numblock) && ts->recv) { 
			ts->recv(ts, pck->block, pck->data, p->tot_len - 4);
			ts->numblock++;
		}
		tftp_ack(ts, pck->block, port);
	}
	pbuf_free(p);
}


err_t tftp_start(struct tftp_server *ts, struct ip_addr *addr, int port)
{
	err_t err;
	memset(ts, 0x0, sizeof(struct tftp_server));
	ts->out = udp_new();
	if (!ts->out)
		return ERR_MEM;
	
	ts->addr.addr = ipaddr_addr("192.168.1.215");
	ts->port = port;

	err = udp_bind(ts->out, addr, port); 
	if (err != ERR_OK) {
		udp_remove(ts->out);
		return err;
	}

	udp_recv(ts->out, udp_tftp_recv, ts);

	return ERR_OK;	
}

void tftp_stop(struct tftp_server *ts)
{
	udp_remove(ts->out);
}

void tftp_recv(struct tftp_server *ts, 
	void (*recv)(struct tftp_server *ts, int num_block, char* buf, int len)) 
{
	ts->recv = recv;	
}

void tftp_err(struct tftp_server *ts, 
	      void (*err)(struct tftp_server *ts, int errcode, char* text))
{
	ts->err = err;	
}

