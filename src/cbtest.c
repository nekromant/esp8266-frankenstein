
// gcc -g -Os -Wall -Wextra -I../include cb.c cbtest.c -o cbtest && ./cbtest
// gcc -g -O3 -Wall -Wextra -I../include cb.c cbtest.c -o cbtest && ./cbtest

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "cb.h"

#define RFRSH	5	// refresh display

#define SZlog2 15

#define R(x)	((rand() % ((x) - 1)) + 1)

char buf [1<<SZlog2];
cb_t cb = CB_INIT(buf, SZlog2);

void info (cb_t* cb)
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
		fprintf(stderr, "syntax: %s C|N\n", argv[0]);
		fprintf(stderr, "	C: perf / R/W copy functions\n");
		fprintf(stderr, "	N: perf / R/W no copy functions\n");
		fprintf(stderr, "	D: perf / R no copy / W copy functions\n");
		fprintf(stderr, "	M: perf / R copy / W no copy functions\n");
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
		if (argv[1][0] == 'C' || argv[1][0] == 'D')
		{
			nw = R(NW);
			for (i = 0; !cb_is_full(&cb) && i < nw; i++)
			{
				sz = (rand() % (SZ - 1)) + 1;
				//memcpy(u, t, sz); // simulate data generation to be written
				ret = cb_write(&cb, t, sz); // write data (copy)
				for (j = 0; j < ret; j++)
					tot += t[j]; // sum
				size += ret;
			}
		}
		else 
		{
			nw = R(NW);
			for (i = 0; !cb_is_full(&cb) && i < nw; i++)
			{
				char* ptr;
				sz = (rand() % (SZ - 1)) + 1;
				ret = cb_write_ptr(&cb, &ptr, sz); // get write ptr
				for (j = 0; j < ret; j++)
					tot += (ptr[j] = t[j]); // write data
				size += ret;
			}
		}

		if (argv[1][0] == 'C' || argv[1][0] == 'M')
		{
			nr = R(NR);
			for (i = 0; !cb_all_is_read(&cb) && i < nr; i++)
			{
				sz = (rand() % (SZ - 1)) + 1;
				ret = cb_read(&cb, u, sz); // read data (copy)
				for (j = 0; j < ret; j++)
					tot -= u[j]; // sum
				size += ret;
			}
		}
		else
		{
			nr = R(NR);
			ack = 0;
			for (i = 0; !cb_all_is_read(&cb) && i < nr; i++)
			{
				char* ptr;
				sz = (rand() % (SZ - 1)) + 1;
				ret = cb_read_ptr(&cb, &ptr, sz); // get read ptr
				for (j = 0; j < ret; j++)
					tot -= ptr[j]; // read data & sum
				ack += ret;
				size += ret;
			}
			cb_ack(&cb, ack); // data not useful anymore
		}
		
	
		if (cb_is_empty(&cb))
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
