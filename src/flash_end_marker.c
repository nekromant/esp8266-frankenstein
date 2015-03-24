#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"
#include <generic/macros.h>

static __attribute__ ((used))	
__attribute__((section(".firmware_end_marker"))) uint32_t flash_ends_here;

/* 
   Since flash's placed @ 0x40200000 we can easily calculate where to place user data
*/

uint32_t fr_get_firmware_size()
{
	return (((uint32_t) &flash_ends_here) + sizeof(uint32_t) - 0x40200000);
}

uint32_t fr_fs_flash_offset()
{
	uint32_t off = fr_get_firmware_size();
	off += SPI_FLASH_SEC_SIZE;
	off &= ~(SPI_FLASH_SEC_SIZE - 1);
	return off;
}

void *fr_fs_physaddr()
{
	return (void *) (fr_fs_flash_offset() + 0x40200000);
}
