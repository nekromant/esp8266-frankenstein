#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "microrl.h"
#include "console.h"
#include "ping.h"

#include <stdlib.h>
#include <generic/macros.h>
#include <sensorlogger.h>
#include <main.h>
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"

#include "config_store.h"

#define BUFLEN 4096
struct webcmdline_data {
	char buf[BUFLEN];
	int pos;
};

static struct webcmdline_data *wcmdline_inst;

static void wcmdline_charhandler(void* arg, char c)
{
		struct webcmdline_data *d = arg;
		d->buf[d->pos++] = c;
		d->buf[d->pos] = 0;
}


static int wcmd_printf (const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);

	ret = vsprintf(&wcmdline_inst->buf[wcmdline_inst->pos], fmt, ap);
	wcmdline_inst->pos += ret;

	va_end(ap);
	return ret;
}

static void wcmdline_init()
{
	console_printf("Switching to web-based console\n");
	console_printf = wcmd_printf;
	struct webcmdline_data *d = os_malloc(sizeof(*d));
	d->pos=0;
	d->buf[0]=0;
	wcmdline_inst = d;
}

static int commandline(HttpdConnData *connData, char *token, void **arg)
{
	if (!wcmdline_inst)
		wcmdline_init();
	console_write(connData->postBuff, strlen(connData->postBuff));
	console_write("\r\n", 2);
	if (wcmdline_inst->pos) {
		espconn_send(connData->conn, wcmdline_inst->buf, wcmdline_inst->pos);
		wcmdline_inst->pos = 0;
	}
	return HTTPD_CGI_DONE;
}


static int webcmdline_start(HttpdConnData *connData, char *token, void **arg)
{
	return HTTPD_CGI_DONE;
	//console_set_charhandler
}


HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.html"},
//	{"/flash.bin", cgiReadFlash, NULL},
//	{"/led.tpl", cgiEspFsTemplate, tplLed},
//	{"/index.tpl", cgiEspFsTemplate, tplCounter},
//	{"/led.cgi", cgiLed, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

//	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
//	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
//	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
//	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
//	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
//	{"/wifi/setmode.cgi", cgiWifiSetMode, NULL},

	// Mky stuff
	{"/showinputs.cgi", cgiShowInputs, NULL},
	{"/commandline", commandline, tplAction},
	{"/commandline/start", webcmdline_start, tplAction},
//	{"/status", cgiEspFsTemplate, "/status.tpl"},
//	{"/showinputs.cgi", cgiShowInputs, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


FR_CONSTRUCTOR(http_service)
{

	console_printf("httpd: Starting http server on port 80\n");
	httpdInit(builtInUrls, 80);
}
