#include "plat_types.h"
#include "cmsis.h"

#ifndef __ARM_ARCH_ISA_ARM
int dwt_enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    return 0;
}

int dwt_disable(void)
{
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    return 0;
}
#endif
