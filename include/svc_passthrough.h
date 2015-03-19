
#ifndef _SVC_PASSTHROUGH_H_
#define _SVC_PASSTHROUGH_H_

#include "tcpservice.h"

extern tcpservice_t* svc_passthrough;

void passthrough_start (int port);
void passthrough_send (char c);

#endif // _SVC_PASSTHROUGH_H
