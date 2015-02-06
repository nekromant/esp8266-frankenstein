#ifndef HELPERS_H
#define HELPERS_H


/* boilerplate for copypaste: 
 * Don't wrap in macro - will screw up doxygen

 const char ICACHE_FLASH_ATTR *id_to_(int id);
 int ICACHE_FLASH_ATTR id_from_(const char *id);

*/

const char ICACHE_FLASH_ATTR *id_to_wireless_mode(int id);
int ICACHE_FLASH_ATTR id_from_wireless_mode(const char *id);

const char ICACHE_FLASH_ATTR *id_to_encryption_mode(int id);
int ICACHE_FLASH_ATTR id_from_encryption_mode(const char *id);

const char ICACHE_FLASH_ATTR *id_to_iface_name(int id);
int ICACHE_FLASH_ATTR id_from_iface_name(const char *id);

const char ICACHE_FLASH_ATTR *id_to_iface_description(int id);
int ICACHE_FLASH_ATTR id_from_iface_description(const char *id);

const char ICACHE_FLASH_ATTR *id_to_sta_state(int id);
int ICACHE_FLASH_ATTR id_from_sta_state(const char *id);

unsigned long  skip_atoul(const char **s);

#endif
