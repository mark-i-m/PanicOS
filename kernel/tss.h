#ifndef _TSS_H_
#define _TSS_H_

#include "stdint.h"

class TSS {
public:
    static void init(void);
    static void esp0(uint32_t v);
};

extern "C" void tss_esp0(uint32_t v); 

#endif
