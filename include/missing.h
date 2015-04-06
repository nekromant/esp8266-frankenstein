
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

#endif // _MISSING_H_
