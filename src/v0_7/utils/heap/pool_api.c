#include "heap_api.h"
#include "hal_trace.h"
#include "plat_addr_map.h"

#define SYS_MEM_POOL_RESERVED_SIZE          512

extern uint8_t __mem_pool_start__[];
extern uint8_t __mem_pool_end__[];

static uint32_t syspool_size = 0;
static uint32_t syspool_used = 0;

uint32_t syspool_original_size(void)
{
    ASSERT((__mem_pool_start__ + SYS_MEM_POOL_RESERVED_SIZE) < __mem_pool_end__,
        "%s: mem pool size too small: start=%p end=%p reserved_size=%u",
        __func__, __mem_pool_start__, __mem_pool_end__, SYS_MEM_POOL_RESERVED_SIZE);
    return __mem_pool_end__ - __mem_pool_start__ - SYS_MEM_POOL_RESERVED_SIZE;
}

void syspool_init()
{
    syspool_size = syspool_original_size();
    syspool_used = 0;
    memset(__mem_pool_start__,0,syspool_size);
    TRACE(2,"syspool_init: %p,0x%x",__mem_pool_start__,syspool_size);
}

void syspool_init_specific_size(uint32_t size)
{
    syspool_size = syspool_original_size();
    syspool_used = 0;
    TRACE(2, "syspool_init_specific_size: %d/%d",size, syspool_size);
    if(size<syspool_size)
        syspool_size = size;
    memset(__mem_pool_start__,0,syspool_size);
    TRACE(2, "syspool_init_specific_size: %p,0x%x", __mem_pool_start__, size);
}

uint8_t* syspool_start_addr(void)
{
    return __mem_pool_start__;
}

uint32_t syspool_total_size(void)
{
    return syspool_size;
}

int syspool_free_size()
{
    return syspool_size - syspool_used;
}

int syspool_get_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    buff_size_free = syspool_free_size();

    if (size % 4){
        size = size + (4 - size % 4);
    }
    ASSERT (size <= buff_size_free, "System pool in shortage! To allocate size %d but free size %d.",
        size, buff_size_free);
    *buff = __mem_pool_start__ + syspool_used;
    syspool_used += size;
    TRACE(0, "[%s] ptr=%p size=%u free=%u user=%p", __func__, *buff, size, buff_size_free, __builtin_return_address(0));
    return buff_size_free;
}

int syspool_get_available(uint8_t **buff)
{
    uint32_t buff_size_free;
    buff_size_free = syspool_free_size();

    TRACE(2, "[%s] free=%d", __func__, buff_size_free);
    if (buff_size_free < 8)
        return -1;
    if (buff != NULL)
    {
        *buff = __mem_pool_start__ + syspool_used;
        syspool_used += buff_size_free;
    }
    return buff_size_free;
}

#if defined(A2DP_LDAC_ON)
int syspool_force_used_size(uint32_t size)
{
    return syspool_used  = size;
}
#endif

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)

extern uint8_t __cp_mem_pool_start__[];
extern uint8_t __cp_mem_pool_end__[];

static uint32_t cp_pool_size = 0;
static uint32_t cp_pool_used = 0;

void cp_pool_init(void)
{
    cp_pool_size = __cp_mem_pool_end__ - __cp_mem_pool_start__;
    cp_pool_used = 0;
    TRACE(5,"[CP] %s start %p end %p size 0x%x %d",__FUNCTION__,
            __cp_mem_pool_start__,
            __cp_mem_pool_end__,cp_pool_size,cp_pool_size);
    if (cp_pool_size) {
        memset(__cp_mem_pool_start__, 0, cp_pool_size);
    }
}

int cp_pool_free_size(void)
{
    return cp_pool_size - cp_pool_used;
}

int cp_pool_get_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    size = (size + 3) & ~3;
    buff_size_free = cp_pool_free_size();

    if (size <= buff_size_free) {
        *buff = __cp_mem_pool_start__ + cp_pool_used;
        cp_pool_used += size;
    } else {
        *buff = NULL;
    }
    TRACE(5,"[CP] %s get %d used %d free 0x%x %d",__FUNCTION__,
            size,cp_pool_used,buff_size_free,buff_size_free);
    return buff_size_free;
}

uint8_t* cp_pool_start_addr(void)
{
    return __cp_mem_pool_start__;
}

uint32_t cp_pool_total_size(void)
{
    return cp_pool_size;
}

#endif

