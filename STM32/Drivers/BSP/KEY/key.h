/**
 ****************************************************************************************************
 * @file        key.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-19
 * @brief       按键输入 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20200419
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 引脚 定义 */

#define KEY0_GPIO_PORT                  GPIOC
#define KEY0_GPIO_PIN                   GPIO_PIN_13
#define KEY0_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)  

#define KEY1_GPIO_PORT                  GPIOC
#define KEY1_GPIO_PIN                   GPIO_PIN_14
#define KEY1_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)  

#define KEY2_GPIO_PORT                  GPIOC
#define KEY2_GPIO_PIN                   GPIO_PIN_15
#define KEY2_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0) 

#define KEY3_GPIO_PORT                  GPIOB
#define KEY3_GPIO_PIN                   GPIO_PIN_6
#define KEY3_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 

#define KEY4_GPIO_PORT                  GPIOB
#define KEY4_GPIO_PIN                   GPIO_PIN_1
#define KEY4_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 

#define KEY5_GPIO_PORT                  GPIOA
#define KEY5_GPIO_PIN                   GPIO_PIN_6
#define KEY5_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0) 

#define KEY6_GPIO_PORT                  GPIOA
#define KEY6_GPIO_PIN                   GPIO_PIN_7
#define KEY6_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0) 

#define KEY7_GPIO_PORT                  GPIOB
#define KEY7_GPIO_PIN                   GPIO_PIN_0
#define KEY7_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 

/******************************************************************************************/

#define KEY0        HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)     /* 读取KEY0引脚 */
#define KEY1        HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)     /* 读取KEY1引脚 */
#define KEY2        HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)     /* 读取KEY2引脚 */
#define KEY3        HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN)     /* 读取KEY3引脚 */
#define KEY4        HAL_GPIO_ReadPin(KEY4_GPIO_PORT, KEY4_GPIO_PIN)     /* 读取KEY3引脚 */
#define KEY5        HAL_GPIO_ReadPin(KEY5_GPIO_PORT, KEY5_GPIO_PIN)     /* 读取KEY3引脚 */
#define KEY6        HAL_GPIO_ReadPin(KEY6_GPIO_PORT, KEY6_GPIO_PIN)     /* 读取KEY3引脚 */
#define KEY7        HAL_GPIO_ReadPin(KEY7_GPIO_PORT, KEY7_GPIO_PIN)     /* 读取KEY3引脚 */


#define KEY0_PRES    1              /* KEY0按下 */
#define KEY1_PRES    2              /* KEY1按下 */
#define KEY2_PRES    3              /* KEY2按下 */
#define KEY3_PRES    4              /* KEY3按下 */
#define KEY4_PRES    5              /* KEY3按下 */
#define KEY5_PRES    6              /* KEY3按下 */
#define KEY6_PRES    7              /* KEY3按下 */
#define KEY7_PRES    8              /* KEY3按下 */

void key_init(void);                /* 按键初始化函数 */
uint8_t key_scan(uint8_t mode);     /* 按键扫描函数 */

#endif


















