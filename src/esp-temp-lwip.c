/**
 * Finds the appropriate network interface for a source IP address. It
 * searches the list of network interfaces linearly. A match is found
 * if the masked IP address of the network interface equals the masked
 * IP address given to the function.
 *
 * @param source the sourcination IP address for which to find the route
 * @return the netif on which to send to reach source
 */

struct netif *
ip_router(ip_addr_t *dest, ip_addr_t *source){
	struct netif *netif;
	/* iterate through netifs */
  	for(netif = netif_list; netif != NULL; netif = netif->next) {
	    /* network mask matches? */
		
		if (netif_is_up(netif)) {
	      if (ip_addr_netcmp(dest, &(netif->ip_addr), &(netif->netmask))) {
	        /* return netif on which to forward IP packet */
	        return netif;
	      }
	    }

		if (netif_is_up(netif)) {
	      if (ip_addr_netcmp(source, &(netif->ip_addr), &(netif->netmask))) {
	        /* return netif on which to forward IP packet */
	        return netif;
	      }
	    }
  	}

	if ((netif_default == NULL) || (!netif_is_up(netif_default))) {
	    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip_route: No route to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
	      ip4_addr1_16(dest), ip4_addr2_16(dest), ip4_addr3_16(dest), ip4_addr4_16(dest)));
	    IP_STATS_INC(ip.rterr);
	    snmp_inc_ipoutnoroutes();
	    return NULL;
  	}
  	/* no matching netif found, use default netif */
  	return netif_default;
}

