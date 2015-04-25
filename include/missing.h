
#ifndef _MISSING_H_
#define _MISSING_H

//#error remove dependencies to missing.h, it is now empty :)

// all of this should go to antares

#include "user_interface.h"
// this should go to antares user_interface.h
struct ip_info {
    struct ip_addr ip;
    struct ip_addr netmask;
    struct ip_addr gw;
};

void ets_intr_lock (void);
void ets_intr_unlock (void);

#endif // _MISSING_H_
