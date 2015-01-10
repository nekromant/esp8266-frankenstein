#ifndef LIB_TFTP_H
#define LIB_TFTP_H


struct tftp_server { 
	int pos; 
	int port; 
	struct udp_pcb *out; 
	struct ip_addr addr; 
	uint16_t pblen;
	int numblock;
	void (*recv)(struct tftp_server *ts, int num_block, char* buf, int len);
	void (*err)(struct tftp_server *ts, int errcode, char* text);
	void *userdata;
};

err_t tftp_start(struct tftp_server *ts, struct ip_addr *addr, int port);
void tftp_stop(struct tftp_server *ts);

void tftp_recv(struct tftp_server *ts, 
	void (*recv)(struct tftp_server *ts, int num_block, char* buf, int len));

void tftp_err(struct tftp_server *ts, 
	      void (*err)(struct tftp_server *ts, int errcode, char* text));

void tftp_request(struct tftp_server *ts, char* host, char* dir, char *fname);


#endif

