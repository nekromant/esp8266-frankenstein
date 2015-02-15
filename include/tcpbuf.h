
#ifndef _TCPBUF_H_
#define _TCPBUF_H_

#include "strbuf.h"

typedef struct
{
	int sbwrite;
	int sbread;
	strbuf strbufs [2];
} tcpbuf;

#define TCPBUF_INIT { .sbwrite = 0, .sbread = 0, .strbufs = { STRBUF_INIT, STRBUF_INIT }, }

#define TB(tb,X) LOGSERIAL(LOG_ERR, X " - 0: s=%d l=%d u=%d 1: s=%d l=%d u=%d - r=%d w=%d", tb->strbufs[0].size, tb->strbufs[0].len, tb->strbufs[0].uptr, tb->strbufs[1].size, tb->strbufs[1].len, tb->strbufs[1].uptr, tb->sbread, tb->sbwrite)

#define tcpbuf_write(tb,pcb,data,len,flags) (strbuf_memcpy(tcpbuf_wbuf(tb), data, len) == -1? ERR_MEM: ERR_OK)

strbuf* tcpbuf_wbuf 		(tcpbuf* tb);
strbuf* tcpbuf_rbuf 		(tcpbuf* tb);
err_t	tcpbuf_send 		(tcpbuf* tb, struct tcp_pcb *pcb);

void	tcp_log_err		(err_t err);

#endif // _TCPBUF_H_
