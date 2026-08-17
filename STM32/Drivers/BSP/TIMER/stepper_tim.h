/**
 ****************************************************************************************************
 * @file        stepper_tim.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-19
 * @brief       定时器 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211019
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __STEPPER_TIM_H
#define __STEPPER_TIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 高级定时器 定义 */

 /* TIMX PWM 定义 
 * 默认使用的是TIM8_CH1.
 * 注意: 通过修改这几个宏定义, 可以支持TIM1/TIM8定时器
 */
#define ATIM_TIMX_PWM_CH1_GPIO_PORT            GPIOA
#define ATIM_TIMX_PWM_CH1_GPIO_PIN             GPIO_PIN_8
#define ATIM_TIMX_PWM_CH1_GPIO_CLK_ENABLE()    do{  __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)     /* PI口时钟使能 */

#define ATIM_TIMX_PWM_CH2_GPIO_PORT            GPIOA
#define ATIM_TIMX_PWM_CH2_GPIO_PIN             GPIO_PIN_9
#define ATIM_TIMX_PWM_CH2_GPIO_CLK_ENABLE()    do{  __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)     /* PI口时钟使能 */

//#define ATIM_TIMX_PWM_CH3_GPIO_PORT            GPIOI
//#define ATIM_TIMX_PWM_CH3_GPIO_PIN             GPIO_PIN_7
//#define ATIM_TIMX_PWM_CH3_GPIO_CLK_ENABLE()    do{  __HAL_RCC_GPIOI_CLK_ENABLE(); }while(0)     /* PI口时钟使能 */

#define ATIM_TIMX_PWM_CH4_GPIO_PORT            GPIOA
#define ATIM_TIMX_PWM_CH4_GPIO_PIN             GPIO_PIN_11
#define ATIM_TIMX_PWM_CH4_GPIO_CLK_ENABLE()    do{  __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)     /* PC口时钟使能 */

//#define ATIM_TIMX_PWM_CHY_GPIO_AF              GPIO_AF3_TIM8

#define ATIM_TIMX_PWM                           TIM1
#define ATIM_TIMX_INT_IRQn                      TIM1_CC_IRQn
#define ATIM_TIMX_INT_IRQHandler                TIM1_CC_IRQHandler
#define ATIM_TIMX_PWM_CH1                       TIM_CHANNEL_1                                    /* 通道1 */
#define ATIM_TIMX_PWM_CH2                       TIM_CHANNEL_2                                    /* 通道2 */
//#define ATIM_TIMX_PWM_CH3                      TIM_CHANNEL_3                                    /* 通道3 */
#define ATIM_TIMX_PWM_CH4                       TIM_CHANNEL_4                                    /* 通道4 */

#define ATIM_TIMX_PWM_CHY_CLK_ENABLE()         do{ __HAL_RCC_TIM1_CLK_ENABLE(); }while(0)       /* TIM1 时钟使能 */


extern TIM_HandleTypeDef g_atimx_handle;                                                        /* 定时器x句柄 */
extern TIM_OC_InitTypeDef g_atimx_oc_chy_handle; 
/******************************************************************************************/

void atim_timx_oc_chy_init(uint16_t arr, uint16_t psc);                                         /* 高级定时器 PWM初始化函数 */

#endif

















