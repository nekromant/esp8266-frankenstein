#include "lwip/netif.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/tcp.h"
#include "lwip/ip.h"
#include "lwip/init.h"
#include "ets_sys.h"
#include "os_type.h"
//#include "os.h"
#include "lwip/mem.h"

#include "lwip/app/espconn_tcp.h"
#include "lwip/app/espconn_udp.h"
#include "lwip/app/espconn.h"


void set_dhcp_hostname(char *hname)
{
	char name[4] = "if1";
	console_printf(name);

	struct netif *i = netif_find("ew0");
//	i->hostname = hname;
	/* LWIP compiled with no hostname support
	   FUCK!
	*/
	
	for (name[0] = 'a' ; name[0] < 'z' ; name[0]++) 
		for (name[1] = 'a' ; name[1] < 'z' ; name[1]++) {	
			i = netif_find(name);
			if (i)
				console_printf("found: %s\n", name);
		}
}
