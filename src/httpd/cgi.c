/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "espmissingincludes.h"
#include <ip_addr.h>
#include "sysex.h"


//cause I can't be bothered to write an ioGetLed()
static char currLedState=0;

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}



//Template code for the led page.
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

static long hitCounter=0;

//Template code for the counter on the index page.
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}


//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	//Send 1K of flash per call. We will get called again if we haven't sent 512K yet.
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

//Template code for the action page.
void ICACHE_FLASH_ATTR tplAction(HttpdConnData *connData, char *token, void **arg) {
	char apikey[128];
	char buff[1024];
	
	httpdFindArg(connData->postBuff, "apikey", apikey, sizeof(apikey));

	int len=httpdFindArg(connData->postBuff, "apikey", buff, sizeof(buff));
	if (token==NULL || len == 0) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "result")==0) {
		if (os_strcmp(apikey, "bfsc7w6y") == 0) {
			ioLed(2500);
			os_strcpy(buff, "true");
		} else {
			os_strcpy(buff, "false");
		}
	}

	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
int ICACHE_FLASH_ATTR cgiShowInputs(HttpdConnData *connData) {
	int len;
	char buff[1024];
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	// output input data
	len=os_sprintf(buff, "{\n \"result\": { \n\"count\": \"%d\", \n\"inputs\": [\n", INPUT_COUNT);
	espconn_sent(connData->conn, (uint8 *)buff, len);
	
	for(int i = 0; i < INPUT_COUNT; i++) {
		int enid = (Inputs[i].id+1);
		len=os_sprintf(buff, "{\"input\": \"INPUT_%s%d\", \"value\": \"%d\"}%s\n", 
				(enid < 10 )?"0":"" , enid, Inputs[i].value, (i==INPUT_COUNT-1)?"":"," );
		espconn_sent(connData->conn, (uint8 *)buff, len);
	}

	len=os_sprintf(buff, "]\n}\n}\n");
	espconn_sent(connData->conn, (uint8 *)buff, len);

	return HTTPD_CGI_DONE;
}

//Template code for the WLAN page.
void ICACHE_FLASH_ATTR tplStatus(HttpdConnData *connData, char *token, void **arg) {
	char buff[1024];
	int x;
	static struct station_config stconf;
	if (token==NULL) return;
	wifi_station_get_config(&stconf);

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "WiFiMode")==0) {
		x=wifi_get_opmode();
		if (x==1) os_strcpy(buff, "Client");
		if (x==2) os_strcpy(buff, "SoftAP");
		if (x==3) os_strcpy(buff, "STA+AP");
	} else if (os_strcmp(token, "currSsid")==0) {
		os_strcpy(buff, (char*)stconf.ssid);
	} else if (os_strcmp(token, "WiFiPasswd")==0) {
		os_strcpy(buff, (char*)stconf.password);
	} else if (os_strcmp(token, "WiFiapwarn")==0) {
		x=wifi_get_opmode();
		if (x==2) {
			os_strcpy(buff, "<b>Can't scan in this mode.</b> Click <a href=\"setmode.cgi?mode=3\">here</a> to go to STA+AP mode.");
		} else {
			os_strcpy(buff, "Click <a href=\"setmode.cgi?mode=2\">here</a> to go to standalone AP mode.");
		}
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
}



//Cgi that turns the LED on or off according to the 'led' param in the POST data
/*
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}
*/