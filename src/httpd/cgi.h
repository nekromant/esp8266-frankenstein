#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
int cgiReadFlash(HttpdConnData *connData);
void tplCounter(HttpdConnData *connData, char *token, void **arg);
void tplAction(HttpdConnData *connData, char *token, void **arg);
int cgiShowInputs(HttpdConnData *connData);
void tplStatus(HttpdConnData *connData, char *token, void **arg);

#endif