#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 基本定时器 定义 */

/* TIMX 中断定义 
 * 默认是针对TIM6/TIM7
 * 注意: 通过修改这4个宏定义,可以支持TIM1~TIM8任意一个定时器.
 */
 
#define BTIM_TIMX_INT                       TIM4
#define BTIM_TIMX_INT_IRQn                  TIM4_IRQn
#define BTIM_TIMX_INT_IRQHandler            TIM4_IRQHandler
#define BTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM4_CLK_ENABLE(); }while(0)   /* TIM6 时钟使能 */

#define BTIM_TIM3_INT                       TIM3
#define BTIM_TIM3_INT_IRQn                  TIM3_IRQn
#define BTIM_TIM3_INT_IRQHandler            TIM3_IRQHandler
#define BTIM_TIM3_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)

/******************************************************************************************/

void btim_timx_int_init(uint16_t arr, uint16_t psc);    /* 基本定时器 定时中断初始化函数 */
void btim_tim3_int_init(uint16_t arr, uint16_t psc);
#endif
