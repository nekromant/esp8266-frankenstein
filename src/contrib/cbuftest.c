
// gcc -g -Os -Wall -Wextra -DCBUFTEST -I../../include ../cbuf.c cbuftest.c -o cbuftest && ./cbuftest
// gcc -g -O3 -Wall -Wextra -DCBUFTEST -I../../include ../cbuf.c cbuftest.c -o cbuftest && ./cbuftest

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "cbuf.h"

#define RFRSH	5	// refresh display

#define BSZ (1<<15)

#define R(x)	((rand() % ((x) - 1)) + 1)

char buf [BSZ];
cbuf_t cb = CBUF_INIT(buf, BSZ);

void info (cbuf_t* cb)
{
	printf("(r=%i u=%i w=%i sz=%i empty=%i allread=%i)", (int)cb->read, (int)cb->unread, (int)cb->write, (int)cb->size, (int)cb->empty, cb->allread);
}

int main (int argc, char* argv[])
{
	const char p [] = "/-\\|";
	const char t [] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
	char u [sizeof t];
	#define SZ (sizeof t - 1)
	#define NW 100
	#define NR 100
	
	int i, j, nr, nw, sz, ret, ack, tot, progress;
	unsigned long long size;
	struct timeval start, now, last;
	
	if (argc != 2 || (argv[1][0] != 'C' && argv[1][0] != 'N' && argv[1][0] != 'D' && argv[1][0] != 'M'))
	{
		fprintf(stderr, "syntax: %s C|N|D|M\n", argv[0]);
		fprintf(stderr, "	C= perf: R copy / W copy functions\n");
		fprintf(stderr, "	N= perf: R no copy / W no copy functions\n");
		fprintf(stderr, "	D= perf: R no copy / W copy functions\n");
		fprintf(stderr, "	M= perf: R copy / W no copy functions\n");
		exit(1);
	}
	
	printf("wait %i seconds\r", RFRSH);
	fflush(stdout);
	
	tot = 0;
	size = 0;
	progress = 0;
	gettimeofday(&start, NULL);
	last = start;
	
	while (1)
	{
		// write to buffer

		if (argv[1][0] == 'C' || argv[1][0] == 'D')
		{
			// write copy
			nw = R(NW);
			for (i = 0; !cbuf_is_full(&cb) && i < nw; i++)
			{
				sz = (rand() % (SZ - 1)) + 1;
				//memcpy(u, t, sz); // simulate data generation to be written
				ret = cbuf_write(&cb, t, sz); // write data (copy)
				for (j = 0; j < ret; j++)
					tot += t[j]; // sum
				size += ret;
			}
		}
		else 
		{
			// write no copy
			nw = R(NW);
			for (i = 0; !cbuf_is_full(&cb) && i < nw; i++)
			{
				char* ptr;
				sz = (rand() % (SZ - 1)) + 1;
				ret = cbuf_write_ptr(&cb, &ptr, sz); // get write ptr
				for (j = 0; j < ret; j++)
					tot += (ptr[j] = t[j]); // write data
				size += ret;
			}
		}

		if (argv[1][0] == 'C' || argv[1][0] == 'M')
		{
			// read copy
			nr = R(NR);
			for (i = 0; !cbuf_all_is_read(&cb) && i < nr; i++)
			{
				sz = (rand() % (SZ - 1)) + 1;
				ret = cbuf_read(&cb, u, sz); // read data (copy)
				for (j = 0; j < ret; j++)
					tot -= u[j]; // sum
				size += ret;
			}
		}
		else
		{
			// read no copy
			nr = R(NR);
			ack = 0;
			for (i = 0; !cbuf_all_is_read(&cb) && i < nr; i++)
			{
				char* ptr;
				sz = (rand() % (SZ - 1)) + 1;
				ret = cbuf_read_ptr(&cb, &ptr, sz); // get read ptr
				for (j = 0; j < ret; j++)
					tot -= ptr[j]; // read data & sum
				ack += ret;
				size += ret;
			}
			cbuf_ack(&cb, ack); // data not useful anymore
		}
	
		if (cbuf_is_empty(&cb))
		{
#if 0
			printf("1: ");
			info(&cb);
			printf("\n");
#endif				
			gettimeofday(&now, NULL);
			if (now.tv_sec > RFRSH + last.tv_sec)
			{
				printf("%c %Li Mbits/s - not Mibits/s\r", p[progress = (progress + 1) & 3], (size << 3) / ((now.tv_sec - start.tv_sec) * 1000000 + (now.tv_usec - start.tv_usec)));
				fflush(stdout);
				gettimeofday(&start, NULL);
				last = start;
				size = 0;
			}
				
			assert(tot == 0); // buffer empty -> sum must be 0
		}
	}
	
	return 0;
}
