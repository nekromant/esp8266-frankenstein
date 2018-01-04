

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"

#include "config_store.h"
#include "uart.h"
#include "sysex.h"

static uart_t* uart0;
// static dce_t* dce;
static config_t* config;

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},
//	{"/flash.bin", cgiReadFlash, NULL},
//	{"/led.tpl", cgiEspFsTemplate, tplLed},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
//	{"/led.cgi", cgiLed, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/setmode.cgi", cgiWifiSetMode, NULL},

	// Mky stuff
	{"/action.tpl", cgiEspFsTemplate, tplAction},
	{"/status", cgiEspFsTemplate, "/status.tpl"},
	{"/showinputs.cgi", cgiShowInputs, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


void ICACHE_FLASH_ATTR rx_dce_cb(char c)
{
	//(dce, &c, 1)
    SysexProcessInput(c);
}

void setupInputsAndEncoders(void) {
	
	for(int i = 0; i < INPUT_COUNT; i++) {
		char buffer[100];
		if(i < 10) {
			os_sprintf(buffer, "INPUT_0%d", i);
		} else {
			os_sprintf(buffer, "INPUT_%d", i);
		}
		Inputs[i].id = i;
		Inputs[i].label = buffer;
	}

	for(int i = 0; i < ENCODER_COUNT; i++) {
		char buffer[100];
		if(i < 10) {
			os_sprintf(buffer, "ENCODER_0%d", i);
		} else {
			os_sprintf(buffer, "ENCODER_%d", i);
		}
		Encoders[i].id = i;
		Encoders[i].label = buffer;
	}
}

typedef void (*uart_rx_handler_t)(char);

//Main routine. Initialize stdout, the I/O and the webserver and we're done.
void user_init(void) {
	stdoutInit();
	ioInit();
	httpdInit(builtInUrls, 80);
	// os_printf("\nReady\n");
	
	setupInputsAndEncoders();
	config = config_init();
    uart0 = uart0_init(config->baud_rate, &rx_dce_cb);
    uart_set_debug(0);

}
