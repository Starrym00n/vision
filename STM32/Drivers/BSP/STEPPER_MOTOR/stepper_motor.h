/**
 ****************************************************************************************************
 * @file        stepper_motor.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-14
 * @brief       步进电机 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F407电机开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211014
 * 第一次发布
 *
 ****************************************************************************************************
 */
#ifndef __STEPPER_MOTOR_H
#define __STEPPER_MOTOR_H

#include "./SYSTEM/sys/sys.h"
/******************************************************************************************/
/* 步进电机参数相关宏 */
#define PULSE_REV       1600.0              /* 每圈脉冲数（细分数8） */
#define MAX_STEP_ANGLE  0.225               /* 最小步距(1.8/PULSE_REV) */

typedef struct
{
    int angle ;                             /* 设置需要旋转的角度 */
    uint8_t dir;                            /* 方向 */
    uint8_t en;                             /* 使能 */
    volatile uint32_t pulse_count;          /* 脉冲个数记录 */
    volatile int add_pulse_count;           /* 脉冲个数累计 */  
} STEPPER_MOTOR;

extern  STEPPER_MOTOR g_stepper1;
extern  STEPPER_MOTOR g_stepper2;
extern  STEPPER_MOTOR g_stepper4;
enum dir
{
    CCW = 0,                                /* 逆时针旋转 */
    CW ,                                    /* 顺时针旋转 */
};  

/******************************************************************************************/
/* 步进电机引脚定义*/

#define STEPPER_MOTOR_1       1             /* 步进电机接口序号 */
#define STEPPER_MOTOR_2       2
//#define STEPPER_MOTOR_3       3
#define STEPPER_MOTOR_4       4
/*     步进电机方向引脚定义     */

#define STEPPER_DIR1_GPIO_PIN                  GPIO_PIN_13
#define STEPPER_DIR1_GPIO_PORT                 GPIOB
#define STEPPER_DIR1_GPIO_CLK_ENABLE()         do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PF口时钟使能 */

#define STEPPER_DIR2_GPIO_PIN                  GPIO_PIN_10
#define STEPPER_DIR2_GPIO_PORT                 GPIOB
#define STEPPER_DIR2_GPIO_CLK_ENABLE()         do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PF口时钟使能 */

//#define STEPPER_DIR3_GPIO_PIN                  GPIO_PIN_2
//#define STEPPER_DIR3_GPIO_PORT                 GPIOB
//#define STEPPER_DIR3_GPIO_CLK_ENABLE()         do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PB口时钟使能 */

#define STEPPER_DIR4_GPIO_PIN                  GPIO_PIN_15
#define STEPPER_DIR4_GPIO_PORT                 GPIOB
#define STEPPER_DIR4_GPIO_CLK_ENABLE()         do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PH口时钟使能 */

/*     步进电机脱机引脚定义     */

#define STEPPER_EN1_GPIO_PIN                   GPIO_PIN_12
#define STEPPER_EN1_GPIO_PORT                  GPIOB
#define STEPPER_EN1_GPIO_CLK_ENABLE()          do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PF口时钟使能 */

#define STEPPER_EN2_GPIO_PIN                   GPIO_PIN_11
#define STEPPER_EN2_GPIO_PORT                  GPIOB
#define STEPPER_EN2_GPIO_CLK_ENABLE()          do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PF口时钟使能 */

//#define STEPPER_EN3_GPIO_PIN                   GPIO_PIN_11
//#define STEPPER_EN3_GPIO_PORT                  GPIOF
//#define STEPPER_EN3_GPIO_CLK_ENABLE()          do{  __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)    /* PF口时钟使能 */
// 
#define STEPPER_EN4_GPIO_PIN                   GPIO_PIN_14
#define STEPPER_EN4_GPIO_PORT                  GPIOB
#define STEPPER_EN4_GPIO_CLK_ENABLE()          do{  __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)    /* PH口时钟使能 */

/*----------------------- 方向引脚控制 -----------------------------------*/
/* 由于我们使用的是共阳极解法，并且硬件对电平做了取反操作，所以当 x = 1 有效，x = 0时无效*/  
#define ST1_DIR(x)    do{ x ? \
                              HAL_GPIO_WritePin(STEPPER_DIR1_GPIO_PORT, STEPPER_DIR1_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(STEPPER_DIR1_GPIO_PORT, STEPPER_DIR1_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)  

#define ST2_DIR(x)    do{ x ? \
                              HAL_GPIO_WritePin(STEPPER_DIR2_GPIO_PORT, STEPPER_DIR2_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(STEPPER_DIR2_GPIO_PORT, STEPPER_DIR2_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)  

//#define ST3_DIR(x)    do{ x ? \
//                              HAL_GPIO_WritePin(STEPPER_DIR3_GPIO_PORT, STEPPER_DIR3_GPIO_PIN, GPIO_PIN_SET) : \
//                              HAL_GPIO_WritePin(STEPPER_DIR3_GPIO_PORT, STEPPER_DIR3_GPIO_PIN, GPIO_PIN_RESET); \
//                          }while(0)  

#define ST4_DIR(x)    do{ x ? \
                              HAL_GPIO_WritePin(STEPPER_DIR4_GPIO_PORT, STEPPER_DIR4_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(STEPPER_DIR4_GPIO_PORT, STEPPER_DIR4_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)  

/*----------------------- 脱机引脚控制 -----------------------------------*/
/* 由于我们使用的是共阳极解法，并且硬件对电平做了取反操作，所以当 x = 1 有效，x = 0时无效*/                          
#define ST1_EN(x)      do{ x ? \
                          HAL_GPIO_WritePin(STEPPER_EN1_GPIO_PORT, STEPPER_EN1_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(STEPPER_EN1_GPIO_PORT, STEPPER_EN1_GPIO_PIN, GPIO_PIN_RESET); \
                        }while(0)    

#define ST2_EN(x)      do{ x ? \
                          HAL_GPIO_WritePin(STEPPER_EN2_GPIO_PORT, STEPPER_EN2_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(STEPPER_EN2_GPIO_PORT, STEPPER_EN2_GPIO_PIN, GPIO_PIN_RESET); \
                        }while(0)    

//#define ST3_EN(x)      do{ x ? \
//                          HAL_GPIO_WritePin(STEPPER_EN3_GPIO_PORT, STEPPER_EN3_GPIO_PIN, GPIO_PIN_SET) : \
//                          HAL_GPIO_WritePin(STEPPER_EN3_GPIO_PORT, STEPPER_EN3_GPIO_PIN, GPIO_PIN_RESET); \
//                        }while(0)    

#define ST4_EN(x)      do{ x ? \
                          HAL_GPIO_WritePin(STEPPER_EN4_GPIO_PORT, STEPPER_EN4_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(STEPPER_EN4_GPIO_PORT, STEPPER_EN4_GPIO_PIN, GPIO_PIN_RESET); \
                        }while(0)                              
                                              
/******************************************************************************************/
/* 外部接口函数*/
void stepper_init(uint16_t arr, uint16_t psc);                          /* 步进电机接口初始化 */
void stepper_star(uint8_t motor_num, uint8_t dir);                      /* 开启步进电机 */
void stepper_stop(uint8_t motor_num);                                   /* 关闭步进电机 */      
//void stepper_set_angle(uint16_t angle, uint8_t dir,uint8_t motor_num);  /* 将角度转换成脉冲个数 */
void motor1_set_angle(int Angle);
void motor2_set_angle(int Angle);
void motor4_set_angle(int Angle);
void stepper1_set_angle(uint16_t angle, uint8_t dir,uint8_t motor_num);
void stepper2_set_angle(uint16_t angle, uint8_t dir,uint8_t motor_num);
void stepper4_set_angle(uint16_t angle, uint8_t dir,uint8_t motor_num);
                        
#endif
