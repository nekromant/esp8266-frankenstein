
#include <stdlib.h>

#include <lwip/tcp.h>

#include "strbuf.h"
#include "tcpbuf.h"

#include "console.h"

strbuf* tcpbuf_wbuf (tcpbuf* tb)
{
	strbuf* sb = &tb->strbufs[tb->sbwrite];
	if (sb->uptr == sb->len)
		// all is swallowed
		sb->uptr = sb->len = 0;
	else if (sb->uptr > (sb->size >> 1) && tb->sbread == tb->sbwrite)
	{
		// sbwrite is growing too much, switch buffer
		sb = &tb->strbufs[tb->sbwrite = 1 - tb->sbwrite];
		if (sb->uptr == sb->len)
			sb->uptr = sb->len = 0;
	}
	return sb;
}

strbuf* tcpbuf_rbuf (tcpbuf* tb)
{
	strbuf* sb = &tb->strbufs[tb->sbread];
	if (sb->uptr == sb->len)
	{
		sb->uptr = sb->len = 0;
		sb = &tb->strbufs[tb->sbread = tb->sbwrite];
	}
	return sb;
}

err_t tcpbuf_send (tcpbuf* tb, struct tcp_pcb *pcb)
{
	strbuf* sb = tcpbuf_rbuf(tb);
	u16_t sendsize = sb->len - sb->uptr;
	if (sendsize)
	{
		// we have data to send
		//u16_t sendmax = MIN(tcp_sndbuf(pcb), 128);
		u16_t sendmax = tcp_sndbuf(pcb);
		if (sendsize > sendmax)
			sendsize = sendmax;
		err_t err;
		if ((err = tcp_write(pcb, sb->buf + sb->uptr, sendsize, /*tcpflags=0=PUSH,NOCOPY*/0)) != ERR_OK)
		{
			tcp_log_err(err);
			return err;
		}
		sb->uptr += sendsize;
	}
	
	return ERR_OK;
}

