#ifndef __DWT_H__
#define __DWT_H__

#ifdef __cplusplus
extern "C" {
#endif

int dwt_enable(void);
int dwt_disable(void);

static inline int dwt_reset_cycle_cnt(void)
{
    DWT->CYCCNT = 0;
    /*start the cycle count*/
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return 0;
}

static inline uint32_t dwt_read_cycle_cnt(void)
{
    uint32_t cnt = DWT->CYCCNT;

    /*stop the cycle count*/
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    return cnt;
}

#ifdef __cplusplus
}
#endif

#endif /* __DWT_H__ */
