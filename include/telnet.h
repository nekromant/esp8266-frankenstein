
#ifndef _TELNET_H
#define _TELNET_H_

void telnet_start(int port);

typedef void (*tcp_continue_f) (void*);
void set_tcp_continue (tcp_continue_f cont, void* arg);

#endif // _TELNET_H_
