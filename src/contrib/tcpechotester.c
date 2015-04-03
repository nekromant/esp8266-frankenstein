
// gcc -Wall -Wextra tcpechotester.c -o tcpechotester

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define BUFLENLOG2 10
#define BUFLEN (1<<(BUFLENLOG2))
static char bufout [BUFLEN];
static char bufin [BUFLEN];

void getsetflag (int sock, int sock2, int level, int flag, int val, const char* name)
{
	static int ret[64];
	socklen_t retlen;
	int locval = val;
	
	retlen = 0;
	ret[0] = 0;
	if (getsockopt(sock, level, flag, ret, &retlen) == -1)
	{
		perror("getsockopt");
		exit(1);
	}
	printf("flag = %s(%i): %i(len=%i) - set it to %i - ", name, flag, *(int*)ret, retlen, locval);
	fflush(stdout);
	if (setsockopt(sock, level, flag, &locval, sizeof(locval)) == -1)
	{
		perror("setsockopt");
		exit(1);
	}
	retlen = sizeof(int);
	*(int*)ret = 0;
	if (getsockopt(sock, level, flag, &ret, &retlen) == -1)
	{
		perror("getsockopt");
		exit(1);
	}
	printf("re-read: %i(len=%i)\n", *(int*)ret, retlen);
	
	if (sock2 > 0 && sock2 != sock)
		getsetflag(sock2, -1, level, flag, val, name);
}
	
void setflag (int sock, int sock2, int level, int flag, int val, const char* name)
{
	int locval = val;

	printf("flag = %s(%i) - set it to %i\n", name, flag, locval);
	if (setsockopt(sock, level, flag, &locval, sizeof(locval)) == -1)
	{
		perror("setsockopt");
		exit(1);
	}
	if (sock2 > 0 && sock2 != sock)
		setflag(sock2, -1, level, flag, val, name);
}
	
int my_socket (void)
{
	int sock;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket()");
		exit(1);
	}
	return sock;
}

void my_bind_listen (int srvsock,  int port)
{
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(srvsock, (struct sockaddr*)&server, sizeof(server)) == -1)
	{
		perror("bind()");
		exit(1);
	}
	
	if (listen(srvsock, 1) == -1)
	{
		perror("listen()");
		exit(1);
	}
	
	printf("bind & listen done.\n");
}

int my_accept (int srvsock)
{
	int clisock;
	socklen_t n;
	struct sockaddr_in client;

	n = sizeof(client);
	if ((clisock = accept(srvsock, (struct sockaddr*)&client, &n)) == -1)
	{
		perror("accept()");
		exit(1);
	}
	
	printf("remote client arrived.\n");
	
	return clisock;
}

void my_connect (const char* servername,  int port,  int sock)
{
	struct sockaddr_in server;
	struct hostent *desc_server;
	
	if ((desc_server = gethostbyname(servername)) == NULL)
	{
		perror("gethostbyname()");
		exit(1);
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	bcopy(desc_server->h_addr, &server.sin_addr, desc_server->h_length);
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
	{
		perror("connect()");
		exit(1);
	}
	
	printf("connected to %s.\n", servername);
}

void my_close (int sock)
{
	if (sock >= 0)
		close(sock);
}

void help (void)
{
	printf("** TCP echo tester - options are:\n"
	       "-h	this help\n"
	       "-p n	set port\n"
	       "-d host	set remote host name\n"
	       "-f	set TCP_NODELAY option\n"
	       "default: localhost:10102\n"
	       "\n");
}

long long bwbps (int diff_s, int diff_us, long long size)
{
	return size / (diff_s + 0.000001 * diff_us) * 8.0;
}

char eng (float* v)
{
	const char unit [] = ".KMGTP";
	unsigned int unitp = 0;
	while (*v > 1024 && unitp < sizeof(unit))
	{
		*v /= 1024;
		unitp++;
	}
	return unit[unitp];
}

void printbw (int diff_s, int diff_us, long long size, const char* head)
{
	float bw = bwbps(diff_s, diff_us, size);
	char u = eng(&bw);
	printf("[%s%g %cibps]", head?:"", bw, u);
}

void printsz (long long sz, const char* head)
{
	float size = sz;
	char u = eng(&size);
	printf("[%s%g %ciB]", head?:"", size, u);
}


void setcntl (int fd, int cmd, int flags, const char* name)
{
	if (fcntl(fd, cmd, flags) == -1)
		perror(name);
}

void echotester (int sock)
{
	setcntl(sock, F_SETFL, O_NONBLOCK, "O_NONBLOCK");
	
	long long sent = 0, recvd = 0, recvdi = 0;
	int ptrsend = 0;
	int ptrrecv = 0;
	struct pollfd pollfd = { .fd = sock, .events = POLLIN | POLLOUT, };
	struct timeval tb, ti, te;
	
	gettimeofday(&tb, NULL);
	ti = tb;
	
	while (1)
	{
		int ret = poll(&pollfd, 1, 1000 /*ms*/);
		
		if (ret == -1)
		{
			perror("poll");
			exit(1);
		}

		if (pollfd.revents & POLLIN)
		{
			ssize_t ret = read(sock, bufin, BUFLEN);
			if (ret == -1)
			{
				perror("read");
				exit(1);
			}
			size_t pr = 0;
			while (ret)
			{
				ssize_t size = ret;
				if (size > BUFLEN - ptrrecv)
					size = BUFLEN - ptrrecv;
				if (memcmp(bufin + pr, bufout + ptrrecv, size) != 0)
				{
					fprintf(stderr, "\ndata differ (sent=%Li revcd=%Li ptrsend=%i ptrrecv=%i ret=%i size=%i)\n", sent, recvd, ptrsend, ptrrecv, (int)ret, (int)size);
					
					int i = 0;
					for (i = 0; i < size; i++)
						if (bufin[i + pr] != bufout[i + ptrrecv])
						{
							printf("%i\n", i);
							break;
						}
					
					exit(1);
				}
				recvd += size;
				recvdi += size;
				ptrrecv = (ptrrecv + size) & (BUFLEN - 1);
				ret -= size;
				pr += size;
			}
		}
		
		if (pollfd.revents & POLLOUT)
		{
			ssize_t size = BUFLEN - ptrsend;
			ssize_t ret = write(sock, bufout + ptrsend, size);
			if (ret == -1)
			{
				perror("write");
				exit(1);
			}
			sent += ret;
			ptrsend = (ptrsend + ret) & (BUFLEN - 1);
		}
		
		gettimeofday(&te, NULL);
		if (te.tv_sec - ti.tv_sec > 1)
		{
			printbw(te.tv_sec - tb.tv_sec, te.tv_usec - tb.tv_usec, recvd, "avg:");
			printbw(te.tv_sec - ti.tv_sec, te.tv_usec - ti.tv_usec, recvdi, "now:");
			printsz(recvd, "size:");
			printf("-----\r"); fflush(stdout);
			ti = te;
			recvdi = 0;
		}
	}

	my_close(sock);
}

void echoserver (int sock)
{
	setcntl(sock, F_SETFL, O_NONBLOCK, "O_NONBLOCK");
	
	int ptrsend = 0;
	int ptrrecv = 0;
	size_t inbuf = 0;
	struct pollfd pollfd = { .fd = sock, .events = POLLIN | POLLOUT, };
	
	while (1)
	{
		pollfd.events =  0;
		if (inbuf < BUFLEN) pollfd.events |= POLLIN;
		if (inbuf) pollfd.events |= POLLOUT;
		int ret = poll(&pollfd, 1, 1000 /*ms*/);
		
		if (ret == -1)
		{
			perror("poll");
			exit(1);
		}

		if (pollfd.revents & POLLIN)
		{
			ssize_t maxrecv = BUFLEN - inbuf;
			if (maxrecv > BUFLEN - ptrrecv)
				maxrecv = BUFLEN - ptrrecv;
			ssize_t ret = read(sock, bufin + ptrrecv, maxrecv);
			if (ret == -1)
			{
				perror("read");
				exit(1);
			}
			inbuf += ret;
			ptrrecv = (ptrrecv + ret) & (BUFLEN - 1);
		}
		
		if (pollfd.revents & POLLOUT)
		{
			ssize_t maxsend = inbuf;
			if (maxsend > BUFLEN - ptrsend)
				maxsend = BUFLEN - ptrsend;
			ssize_t ret = write(sock, bufin + ptrsend, maxsend);
			if (ret == -1)
			{
				perror("write");
				exit(1);
			}
			inbuf -= ret;
			ptrsend = (ptrsend + ret) & (BUFLEN - 1);
		}
	}

	my_close(sock);
}

int main (int argc, char* argv[])
{
	int op;
	const char* host = "localhost";
	int port = 10102;
	int server = 0;
	int i;
	//int nodelay = 0;
	
	while ((op = getopt(argc, argv, "hp:d:fs")) != EOF) switch(op)
	{
		case 'h':
			help();
			return 0;

		case 'p':
			port = atoi(optarg);			
			break;
			
		case 'd':
			host = optarg;
			break;
		
//		case 'f':
//			nodelay = 1;
//			break;

		case 's':
			server = 1;
			break;
		
		default:
			printf("option '%c' not recognized\n", op);
			help();
			return 1;
	}
	
	for (i = 0; i < BUFLEN; i++)
		bufout[i] = random() >> 23;

	int sock = my_socket();
	if (server)
	{
		printf("waiting on port %i\n", port);
		my_bind_listen(sock, port);
		int clisock = my_accept(sock);
		echoserver(clisock);
		close(sock);
	}
	else
	{
		printf("remote host:	%s\n"
		       "port:		%i\n",
		       host, port);
	
		my_connect(host, port, sock);
		echotester(sock);
	}
	
	return 0;
}
