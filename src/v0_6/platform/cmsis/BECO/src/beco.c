#include "plat_types.h"
#include "cmsis.h"
#include "hal_cmu.h"

int set_beco_coprocessor(int enable);

int beco_init(void)
{
    uint32_t lock;

    lock = int_lock();
    hal_cmu_beco_enable();
    set_beco_coprocessor(1);
    int_unlock(lock);

    return 0;
}

int beco_exit(void)
{
    uint32_t lock;

    lock = int_lock();
    set_beco_coprocessor(0);
    hal_cmu_beco_disable();
    int_unlock(lock);

    return 0;
}
