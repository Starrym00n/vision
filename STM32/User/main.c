/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-21
 * @brief       高级定时器输出指定个数PWM 实验
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
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/STEPPER_MOTOR/stepper_motor.h"
#include "./BSP/TIMER/stepper_tim.h"
#include "./BSP/BTIM/btim.h"
#include "oled.h"
#include <math.h>
#include <stdio.h> 

typedef int bool;
#define true 1
#define false 0

extern uint8_t g_run_flag1;
extern uint8_t g_run_flag4;

void set_angle(int motor1,int motor4);  //设置电机旋转角度函数总封装
void set_angle2(int motor2);
void Absorb(void);                      //吸取棋子
void Laydown(void);                     //放下棋子
void Revolve_set1379(int x1,int x2,int y1,int y2);
void Revolve_set2468(int x1,int x2,int y1,int y2);
void ONE_black_1(void);
void ONE_black_2(void);
void ONE_black_3(void);
void ONE_black_4(void);
void ONE_black_5(void);
void ONE_white_1(void);
void ONE_white_2(void);
void ONE_white_3(void);
void ONE_white_4(void);
void ONE_white_5(void);
int checkwin(int board[9]);
bool isMovesLeft(int board[9]);
int minimax(int board[9], int depth, bool isMax);
int findBestMove(int board[9]);

uint8_t color=0;    //0是黑色，1是白色
uint8_t num=1;      //棋子编号
uint8_t position=1; //放置位置
uint8_t start=0;    //执行标志  1执行  0不执行

uint8_t display_buf[20];
uint8_t tt;
uint8_t one_flag=0;

uint32_t a[4][3]={{0,0,0},{0,0,0},{0,0,0},{0,0,0}};  //存放四个棋子的属性
//a[0][]第一步棋子，a[1][]第二步棋子，a[2][]第三步棋子，a[3][]第四步棋子
uint8_t flag=0;
unsigned long int start_time=0;

//第三题
uint8_t flag_3=1;
float JIAO=0.00f;
float JIAO1=0.00f;
float A_mm=0.00f;
float X_mm=0.00f;
float Y_mm=0.00f;
#define PI 3.1415

uint8_t usart_rx_flag=0;
#define FRAME_LENGTH 14
uint8_t rx_buffer[FRAME_LENGTH];
uint8_t jiaodu;

//第四题
uint8_t si_flag=0;
uint8_t si_rx_buffer[FRAME_LENGTH];
int board[9]={0};
int board2[9]={0};

//第五题
uint8_t one_logo=0;

int main(void)
{
    uint8_t key;
    
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9); /* 设置时钟, 72Mhz */
    delay_init(72);                     /* 延时初始化 */
    usart_init(9600);                 /* 串口初始化为115200 */
    led_init();                         /* 初始化LED */
    key_init();                         /* 初始化按键 */
    OLED_Init();
    OLED_Clear();
    sprintf((char *)display_buf,"delay:500ms...");
    OLED_ShowString(2,3,display_buf,16);
    stepper_init(0xFFFF, 72 - 1);
    btim_timx_int_init(100-1,7200-1);
    btim_tim3_int_init(2000-1,7200-1);
    delay_ms(400);
    set_angle2(120);
    delay_ms(100);
    set_angle2((int)(-(g_stepper2.add_pulse_count*MAX_STEP_ANGLE)));
    OLED_Clear();
    
    while (1)
    {
        if(usart_rx_flag==1)
        {
            HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
            if(rx_buffer[0]=='[')
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 3,HAL_MAX_DELAY);
                if(rx_buffer[3]==']')
                {
                    JIAO=(rx_buffer[1]-0x30)*10+rx_buffer[2]-0x30;
                    usart_rx_flag=0;
                }
            }
//            else rx_buffer[0]=0;
        }
        
        key=key_scan(0);
        if(key == KEY2_PRES)
        {
            color=!color;
        }
        else if(key == KEY1_PRES)
        {
            num++;if(num>5){num=1;}
        }
        else if(key == KEY0_PRES)
        {
            position++;if(position>9){position=1;}
        }
        else if(key == KEY3_PRES)
        {
            start=1;
            sprintf((char *)display_buf,"start...");
            OLED_ShowString(0,7,display_buf,12);
        }
        else if(key == KEY4_PRES)
        {
            flag++;
            if(flag==1)
            {
                a[0][0]=color;a[0][1]=num;a[0][2]=position;
            }
            else if(flag==2)
            {
                a[1][0]=color;a[1][1]=num;a[1][2]=position;
            }
            else if(flag==3)
            {
                a[2][0]=color;a[2][1]=num;a[2][2]=position;
            }
            else if(flag==4)
            {
                a[3][0]=color;a[3][1]=num;a[3][2]=position;
            }
            else if(flag==5)
            {
                flag=5;
            }
            sprintf((char *)display_buf,"flag:%d",flag);
            OLED_ShowString(0,6,display_buf,12);
        }
        else if(key == KEY5_PRES)
        {
            usart_rx_flag=!usart_rx_flag; 
        }
        else if(key == KEY6_PRES)
        {
            JIAO=-JIAO;one_logo=1;
            sprintf((char *)display_buf,"JIAO:%d ",(int)JIAO);
            OLED_ShowString(0,0,display_buf,12);
        }
        else if(key == KEY7_PRES)
        {
            si_flag++;LED0(0);
        }
        
        if(start==0)
        {
            sprintf((char *)display_buf," stop   ");
            OLED_ShowString(0,7,display_buf,12);
        }
        sprintf((char *)display_buf,"JIAO:%d   ",(int)JIAO);
        OLED_ShowString(0,0,display_buf,12);
        sprintf((char *)display_buf,"flag_3:%d",(int)flag_3);
        OLED_ShowString(0,1,display_buf,12);
        sprintf((char *)display_buf,"usart_rx_flag:%d",(int)usart_rx_flag);
        OLED_ShowString(0,2,display_buf,12);
        sprintf((char *)display_buf,"color:%d",(int)color);
        OLED_ShowString(0,3,display_buf,12);
        sprintf((char *)display_buf,"num:   %d",(int)num);
        OLED_ShowString(0,4,display_buf,12);
        sprintf((char *)display_buf,"position:%d",(int)position);
        OLED_ShowString(0,5,display_buf,12);
        sprintf((char *)display_buf,"flag:%d",flag);
        OLED_ShowString(0,6,display_buf,12);
        
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIMX_INT)
    {
         
        if(start)
        {
            ONE_black_1();  /*黑色棋子，第1个随机移动*/
            ONE_black_2();  /*黑色棋子，第2个随机移动*/
            ONE_black_3();  /*黑色棋子，第3个随机移动*/
            ONE_black_4();  /*黑色棋子，第4个随机移动*/
            ONE_black_5();  /*黑色棋子，第5个随机移动*/
            ONE_white_1();  /*白色棋子，第1个随机移动*/
            ONE_white_2();  /*白色棋子，第2个随机移动*/
            ONE_white_3();  /*白色棋子，第3个随机移动*/
            ONE_white_4();  /*白色棋子，第4个随机移动*/
            ONE_white_5();  /*白色棋子，第5个随机移动*/
        }
    }
    if(htim->Instance == BTIM_TIM3_INT)//200ms进一次
    {
        if(one_logo==0){
        if(si_flag==1)
        {
            color=0;num=1;position=5;
            ONE_black_1();ONE_black_2();ONE_black_3();ONE_black_4();ONE_black_5();
            LED0(1);
            si_flag=2;
        }
        else if(si_flag==3)
        {
            HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
            if(rx_buffer[0]=='[')
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                if(rx_buffer[10]==']')
                {
                    board[0]=rx_buffer[1]-0x30;
                    board[1]=rx_buffer[2]-0x30;
                    board[2]=rx_buffer[3]-0x30;
                    board[3]=rx_buffer[4]-0x30;
                    board[4]=rx_buffer[5]-0x30;
                    board[5]=rx_buffer[6]-0x30;
                    board[6]=rx_buffer[7]-0x30;
                    board[7]=rx_buffer[8]-0x30;
                    board[8]=rx_buffer[9]-0x30;
                    color=0;num=2;position=findBestMove(board)+1;
                    ONE_black_1();ONE_black_2();ONE_black_3();ONE_black_4();ONE_black_5();
                    LED0(1);
                    si_flag=4;
                }
            }
        }
        else if(si_flag==5)
        {
            HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
            if(rx_buffer[0]=='[')
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                if(rx_buffer[10]==']')
                {
                    board[0]=rx_buffer[1]-0x30;
                    board[1]=rx_buffer[2]-0x30;
                    board[2]=rx_buffer[3]-0x30;
                    board[3]=rx_buffer[4]-0x30;
                    board[4]=rx_buffer[5]-0x30;
                    board[5]=rx_buffer[6]-0x30;
                    board[6]=rx_buffer[7]-0x30;
                    board[7]=rx_buffer[8]-0x30;
                    board[8]=rx_buffer[9]-0x30;
                    color=0;num=3;position=findBestMove(board)+1;
                    ONE_black_1();ONE_black_2();ONE_black_3();ONE_black_4();ONE_black_5();
                    LED0(1);
                    si_flag=6;
                }
            }
        }
        else if(si_flag==7)
        {
            HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
            if(rx_buffer[0]=='[')
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                if(rx_buffer[10]==']')
                {
                    board[0]=rx_buffer[1]-0x30;
                    board[1]=rx_buffer[2]-0x30;
                    board[2]=rx_buffer[3]-0x30;
                    board[3]=rx_buffer[4]-0x30;
                    board[4]=rx_buffer[5]-0x30;
                    board[5]=rx_buffer[6]-0x30;
                    board[6]=rx_buffer[7]-0x30;
                    board[7]=rx_buffer[8]-0x30;
                    board[8]=rx_buffer[9]-0x30;
                    color=0;num=4;position=findBestMove(board)+1;
                    ONE_black_1();ONE_black_2();ONE_black_3();ONE_black_4();ONE_black_5();
                    LED0(1);
                    si_flag=8;
                }
            }
        }
        else if(si_flag==9)
        {
            HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
            if(rx_buffer[0]=='[')
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                if(rx_buffer[10]==']')
                {
                    board[0]=rx_buffer[1]-0x30;
                    board[1]=rx_buffer[2]-0x30;
                    board[2]=rx_buffer[3]-0x30;
                    board[3]=rx_buffer[4]-0x30;
                    board[4]=rx_buffer[5]-0x30;
                    board[5]=rx_buffer[6]-0x30;
                    board[6]=rx_buffer[7]-0x30;
                    board[7]=rx_buffer[8]-0x30;
                    board[8]=rx_buffer[9]-0x30;
                    color=0;num=5;position=findBestMove(board)+1;
                    ONE_black_1();ONE_black_2();ONE_black_3();ONE_black_4();ONE_black_5();
                    LED0(1);
                    si_flag=10;
                }
            }
        }}
        if(one_logo==1)
        {
            if(si_flag==1)
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
                if(rx_buffer[0]=='[')
                {
                    HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                    if(rx_buffer[10]==']')
                    {
                        board[0]=rx_buffer[1]-0x30;
                        board[1]=rx_buffer[2]-0x30;
                        board[2]=rx_buffer[3]-0x30;
                        board[3]=rx_buffer[4]-0x30;
                        board[4]=rx_buffer[5]-0x30;
                        board[5]=rx_buffer[6]-0x30;
                        board[6]=rx_buffer[7]-0x30;
                        board[7]=rx_buffer[8]-0x30;
                        board[8]=rx_buffer[9]-0x30;
                        for(int i=0;i<=8;i++)
                        {
                            if(board[i]==0)board2[i]=0;
                            else if(board[i]==1)board2[i]=2;
                            else if(board[i]==2)board2[i]=1;
                        }
                        color=1;num=1;position=findBestMove(board2)+1;
                        ONE_white_1();ONE_white_2();ONE_white_3();ONE_white_4();ONE_white_5();
                        LED0(1);
                        si_flag=2;
                    }
                }
            }
            else if(si_flag==3)
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
                if(rx_buffer[0]=='[')
                {
                    HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                    if(rx_buffer[10]==']')
                    {
                        board[0]=rx_buffer[1]-0x30;
                        board[1]=rx_buffer[2]-0x30;
                        board[2]=rx_buffer[3]-0x30;
                        board[3]=rx_buffer[4]-0x30;
                        board[4]=rx_buffer[5]-0x30;
                        board[5]=rx_buffer[6]-0x30;
                        board[6]=rx_buffer[7]-0x30;
                        board[7]=rx_buffer[8]-0x30;
                        board[8]=rx_buffer[9]-0x30;
                        for(int i=0;i<=8;i++)
                        {
                            if(board[i]==0)board2[i]=0;
                            else if(board[i]==1)board2[i]=2;
                            else if(board[i]==2)board2[i]=1;
                        }
                        color=1;num=2;position=findBestMove(board2)+1;
                        ONE_white_1();ONE_white_2();ONE_white_3();ONE_white_4();ONE_white_5();
                        LED0(1);
                        si_flag=4;
                    }
                }
            }
            else if(si_flag==5)
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
                if(rx_buffer[0]=='[')
                {
                    HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                    if(rx_buffer[10]==']')
                    {
                        board[0]=rx_buffer[1]-0x30;
                        board[1]=rx_buffer[2]-0x30;
                        board[2]=rx_buffer[3]-0x30;
                        board[3]=rx_buffer[4]-0x30;
                        board[4]=rx_buffer[5]-0x30;
                        board[5]=rx_buffer[6]-0x30;
                        board[6]=rx_buffer[7]-0x30;
                        board[7]=rx_buffer[8]-0x30;
                        board[8]=rx_buffer[9]-0x30;
                        for(int i=0;i<=8;i++)
                        {
                            if(board[i]==0)board2[i]=0;
                            else if(board[i]==1)board2[i]=2;
                            else if(board[i]==2)board2[i]=1;
                        }
                        color=1;num=3;position=findBestMove(board2)+1;
                        ONE_white_1();ONE_white_2();ONE_white_3();ONE_white_4();ONE_white_5();
                        LED0(1);
                        si_flag=6;
                    }
                }
            }
            else if(si_flag==7)
            {
                HAL_UART_Receive(&g_uart1_handle, &rx_buffer[0], 1,HAL_MAX_DELAY); // 启动接收第一个字节
                if(rx_buffer[0]=='[')
                {
                    HAL_UART_Receive(&g_uart1_handle, &rx_buffer[1], 10,HAL_MAX_DELAY);
                    if(rx_buffer[10]==']')
                    {
                        board[0]=rx_buffer[1]-0x30;
                        board[1]=rx_buffer[2]-0x30;
                        board[2]=rx_buffer[3]-0x30;
                        board[3]=rx_buffer[4]-0x30;
                        board[4]=rx_buffer[5]-0x30;
                        board[5]=rx_buffer[6]-0x30;
                        board[6]=rx_buffer[7]-0x30;
                        board[7]=rx_buffer[8]-0x30;
                        board[8]=rx_buffer[9]-0x30;
                        for(int i=0;i<=8;i++)
                        {
                            if(board[i]==0)board2[i]=0;
                            else if(board[i]==1)board2[i]=2;
                            else if(board[i]==2)board2[i]=1;
                        }
                        color=1;num=4;position=findBestMove(board2)+1;
                        ONE_white_1();ONE_white_2();ONE_white_3();ONE_white_4();ONE_white_5();
                        LED0(1);
                        si_flag=8;
                    }
                }
            }
        }
        if(flag==5)
        {
            start_time++;
            if(start_time==1)
            {
                color=a[0][0];num=a[0][1];position=a[0][2];
                start=1;
            }
            else if(start_time==61)
            {
                color=a[1][0];num=a[1][1];position=a[1][2];
                start=1;
            }
            else if(start_time==121)
            {
                color=a[2][0];num=a[2][1];position=a[2][2];
                start=1;
            }
            else if(start_time==181)
            {
                color=a[3][0];num=a[3][1];position=a[3][2];
                start=1;
                flag=0;
                sprintf((char *)display_buf,"flag:%d",flag);
                OLED_ShowString(0,6,display_buf,12);
            }
        }
        
    }
}
//void goto_seat(int a,int b,int c) //1黑2白，第几个棋，目的位置
//{
//    if(a==1)
//    {
//        switch(c)
//        {
//            case 1:color=0;num=b;position=5;
//                break;
//        }
//    }
//}


// 检查棋盘状态：1为黑棋胜利，2为白棋胜利，0为平局，-1为游戏进行中
int checkwin(int board[9]) {
    int win_conditions[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
        {0, 4, 8}, {2, 4, 6}
    };
    for (int i = 0; i < 8; i++) {
        if (board[win_conditions[i][0]] != 0 &&
            board[win_conditions[i][0]] == board[win_conditions[i][1]] &&
            board[win_conditions[i][1]] == board[win_conditions[i][2]]) {
            return board[win_conditions[i][0]];
        }
    }
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) return -1;
    }
    return 0;
}

// 判断棋盘上是否还有空位
bool isMovesLeft(int board[9]) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) {
            return true;
        }
    }
    return false;
}

// 迷你麦克斯算法：评估当前棋盘的最佳得分
int minimax(int board[9], int depth, bool isMax) {
    int score = checkwin(board);
    if (score == 1) return -10 + depth;  // 黑棋胜，返回负分
    if (score == 2) return 10 - depth;   // 白棋胜，返回正分
    if (!isMovesLeft(board)) return 0;   // 平局

    if (isMax) {
        int best = -1000;
        for (int i = 0; i < 9; i++) {
            if (board[i] == 0) {
                board[i] = 2;
                int moveVal = minimax(board, depth + 1, false);
                board[i] = 0;
                if (moveVal > best) {
                    best = moveVal;
                }
            }
        }
        return best;
    } else {
        int best = 1000;
        for (int i = 0; i < 9; i++) {
            if (board[i] == 0) {
                board[i] = 1;
                int moveVal = minimax(board, depth + 1, true);
                board[i] = 0;
                if (moveVal < best) {
                    best = moveVal;
                }
            }
        }
        return best;
    }
}

// 寻找当前棋盘状态下电脑的最佳移动
int findBestMove(int board[9]) {
    int gameStatus = checkwin(board);
    if (gameStatus != -1) {
    	if(gameStatus == 0)return 99;// 游戏已经结束，无需继续寻找最佳移动
        if(gameStatus == 1)return -11; 
        if(gameStatus == 2)return 9;
    }

    int bestVal = -1000;
    int bestMove = -1;
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) {
            board[i] = 2;
            int moveVal = minimax(board, 0, false);
            board[i] = 0;
            if (moveVal > bestVal) {
                bestMove = i;
                bestVal = moveVal;
            }
        }
    }
    return bestMove;
}


void set_angle(int motor1,int motor4)
{
    motor1_set_angle(motor1);
    stepper1_set_angle(g_stepper1.angle, g_stepper1.dir, STEPPER_MOTOR_1);
    motor4_set_angle(motor4);
    stepper4_set_angle(g_stepper4.angle, g_stepper4.dir, STEPPER_MOTOR_4);
    while(g_stepper1.pulse_count);
    while(g_stepper4.pulse_count);
}
void set_angle2(int motor2)
{
    motor2_set_angle(motor2);
    stepper2_set_angle(g_stepper2.angle, g_stepper2.dir, STEPPER_MOTOR_2);
    while(g_stepper2.pulse_count);
}
void Absorb(void)
{
    set_angle2(110);  //下降
    delay_ms(500);
    RELAY0(1);       //吸取
    delay_ms(500);
    set_angle2((int)(-(g_stepper2.add_pulse_count*MAX_STEP_ANGLE)));//上升
}
void Laydown(void)
{
    set_angle2(110);  //下降
    delay_ms(500);
    RELAY0(0);       //放下
    delay_ms(500);
    set_angle2((int)(-(g_stepper2.add_pulse_count*MAX_STEP_ANGLE)));//上升
}
void Revolve_set1379(int x1,int x2,int y1,int y2)
{
    if(flag_3) //顺时针旋转为例
    {
        if(JIAO>0)//顺时针旋转
        {
            JIAO1=(180-JIAO)/2;
            A_mm=sqrt(4050-4050*cos(JIAO*PI/180.0));
            X_mm=A_mm*sin(JIAO1*PI/180.0);
            Y_mm=A_mm*cos(JIAO1*PI/180.0);
            set_angle(x1*(int)(X_mm*8.928),x2*(int)(X_mm*8.928));
            set_angle(y1*(int)(Y_mm*8.928),y2*(int)(Y_mm*8.928));
        }
        else if(JIAO<0)//逆时针旋转
        {
            JIAO=-JIAO;
            JIAO1=(180-JIAO)/2;
            A_mm=sqrt(4050-4050*cos(JIAO*PI/180.0));
            X_mm=A_mm*sin(JIAO1*PI/180.0);
            Y_mm=A_mm*cos(JIAO1*PI/180.0);
            set_angle((-y1)*(int)(X_mm*8.928),(-y2)*(int)(X_mm*8.928));
            set_angle((-x1)*(int)(Y_mm*8.928),(-x2)*(int)(Y_mm*8.928));
        }
    }
}
void Revolve_set2468(int x1,int x2,int y1,int y2)
{
    if(flag_3) //顺时针旋转为例
    {
        if(JIAO>0)//顺时针旋转
        {
            JIAO1=(180-JIAO)/2;
            A_mm=sqrt(2048-2048*cos(JIAO*PI/180.0));
            X_mm=A_mm*sin(JIAO1*PI/180.0);
            Y_mm=A_mm*cos(JIAO1*PI/180.0);
            set_angle(x1*(int)(X_mm*8.928),x2*(int)(X_mm*8.928));
            set_angle(y1*(int)(Y_mm*8.928),y2*(int)(Y_mm*8.928));
        }
        else if(JIAO<0)//逆时针旋转
        {
            JIAO=-JIAO;
            JIAO1=(180-JIAO)/2;
            A_mm=sqrt(2048-2048*cos(JIAO*PI/180.0));
            X_mm=A_mm*sin(JIAO1*PI/180.0);
            Y_mm=A_mm*cos(JIAO1*PI/180.0);
            set_angle((-x1)*(int)(X_mm*8.928),(-x2)*(int)(X_mm*8.928));
            set_angle(y1*(int)(Y_mm*8.928),y2*(int)(Y_mm*8.928));
        }
    }
}
void ONE_white_5(void)
{
    if(color==1&&num==5&&position>=1&&position<=3)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-1370,-1370);//下移
        Absorb();
        set_angle(-420-(3-position)*277,420+(3-position)*277);//左移
        delay_ms(10);
        set_angle(775,775);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==5&&position>=4&&position<=6)
    {
        set_angle(1420,-1420);//右移
        delay_ms(10);
        set_angle(-1370,-1370);//下移
        Absorb();
        set_angle(-425-(6-position)*277,425+(6-position)*277);//左移
        delay_ms(10);
        set_angle(510,510);//上移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==5&&position>=7&&position<=9)
    {
        set_angle(1420,-1420);//右移
        delay_ms(10);
        set_angle(-1370,-1370);//下移
        Absorb();
        set_angle(-425-(9-position)*277,425+(9-position)*277);//左移
        delay_ms(10);
        set_angle(230,230);//上移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_white_4(void)
{
    if(color==1&&num==4&&position>=1&&position<=3)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-1165,-1165);//下移
        Absorb();
        set_angle(-420-(3-position)*277,420+(3-position)*277);//左移
        delay_ms(10);
        set_angle(570,570);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==4&&position>=4&&position<=6)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-1165,-1165);//下移
        Absorb();
        set_angle(-420-(6-position)*277,420+(6-position)*277);//左移
        delay_ms(10);
        set_angle(305,305);//上移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==4&&position>=7&&position<=9)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-1165,-1165);//下移
        Absorb();
        set_angle(-420-(9-position)*277,420+(9-position)*277);//左移
        delay_ms(10);
        set_angle(30,30);//上移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_white_3(void)
{
    if(color==1&&num==3&&position>=1&&position<=3)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-941,-941);//下移
        Absorb();
        set_angle(-420-(3-position)*277,420+(3-position)*277);//左移
        delay_ms(10);
        set_angle(350,350);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==3&&position>=4&&position<=6)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-941,-941);//下移
        Absorb();
        set_angle(-420-(6-position)*277,420+(6-position)*277);//左移
        delay_ms(10);
        set_angle(80,80);//上移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==3&&position>=7&&position<=9)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-941,-941);//下移
        Absorb();
        set_angle(-420-(9-position)*277,420+(9-position)*277);//左移
        delay_ms(10);
        set_angle(-197,-197);//下移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_white_2(void)
{
    if(color==1&&num==2&&position>=1&&position<=3)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-736,-736);//下移
        Absorb();
        set_angle(-420-(3-position)*277,420+(3-position)*277);//左移
        delay_ms(10);
        set_angle(145,145);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==2&&position>=4&&position<=6)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-736,-736);//下移
        Absorb();
        set_angle(-420-(6-position)*277,420+(6-position)*277);//左移
        delay_ms(10);
        set_angle(-120,-120);//下移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==2&&position>=7&&position<=9)
    {
        set_angle(1410,-1410);//右移
        delay_ms(10);
        set_angle(-736,-736);//下移
        Absorb();
        set_angle(-420-(9-position)*277,420+(9-position)*277);//左移
        delay_ms(10);
        set_angle(-397,-397);//下移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_white_1(void)
{
    if(color==1&&num==1&&position>=1&&position<=3)
    {
        set_angle(1400,-1400);//右移
        delay_ms(10);
        set_angle(-540,-540);//下移
        Absorb();
        set_angle(-410-(3-position)*277,410+(3-position)*277);//左移
        delay_ms(10);
        set_angle(-45,-45);//下移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==1&&position>=4&&position<=6)
    {
        set_angle(1400,-1400);//右移
        delay_ms(10);
        set_angle(-540,-540);//下移
        Absorb();
        set_angle(-410-(6-position)*277,410+(6-position)*277);//左移
        delay_ms(10);
        set_angle(-322,-322);//下移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==1&&num==1&&position>=7&&position<=9)
    {
        set_angle(1400,-1400);//右移
        delay_ms(10);
        set_angle(-540,-540);//下移
        Absorb();
        set_angle(-410-(9-position)*277,410+(9-position)*277);//左移
        delay_ms(10);
        set_angle(-600,-600);//下移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_black_5(void)
{
    if(color==0&&num==5&&position>=1&&position<=3)
    {
        set_angle(-1390,-1390);//下移
        Absorb();
        set_angle(435+(position-1)*277,-435-(position-1)*277);//右移
        delay_ms(10);
        set_angle(790,790);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==5&&position>=4&&position<=6)
    {
        set_angle(-1390,-1390);//下移
        Absorb();
        set_angle(435+(position-4)*277,-435-(position-4)*277);//右移
        delay_ms(10);
        set_angle(520,520);//上移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==5&&position>=7&&position<=9)
    {
        set_angle(-1390,-1390);//下移
        Absorb();
        set_angle(435+(position-7)*277,-435-(position-7)*277);//右移
        delay_ms(10);
        set_angle(255,255);//上移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_black_4(void)
{
    if(color==0&&num==4&&position>=1&&position<=3)
    {
        set_angle(-1176,-1176);//下移
        Absorb();
        set_angle(435+(position-1)*277,-435-(position-1)*277);//右移
        delay_ms(10);
        set_angle(581,581);//上移
        if(position==1)
        {
            Revolve_set1379(1,-1,1,1);
        }
        else if(position==2)
        {
            Revolve_set2468(1,-1,-1,-1);
        }
        else if(position==3)
        {
            Revolve_set1379(-1,-1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==4&&position>=4&&position<=6)
    {
        set_angle(-1176,-1176);//下移
        Absorb();
        set_angle(435+(position-4)*277,-435-(position-4)*277);//右移
        delay_ms(10);
        set_angle(310,310);//上移
        if(position==6)
        {
            Revolve_set2468(-1,-1,-1,1);
        }
        else if(position==4)
        {
            Revolve_set2468(1,1,1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==4&&position>=7&&position<=9)
    {
        set_angle(-1176,-1176);//下移
        Absorb();
        set_angle(435+(position-7)*277,-435-(position-7)*277);//右移
        delay_ms(10);
        set_angle(40,40);//上移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_black_3(void)
{
    if(color==0&&num==3&&position==1)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(430,-430);//右移
        delay_ms(10);
        set_angle(375,375);//上移
        Revolve_set1379(1,-1,1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position==2)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(707,-707);//右移
        delay_ms(10);
        set_angle(375,375);//上移
        Revolve_set2468(1,-1,-1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position==3)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(984,-984);//右移
        delay_ms(10);
        set_angle(375,375);//上移
        Revolve_set1379(-1,-1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position==4)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(435,-435);//右移
        delay_ms(10);
        set_angle(110,110);//上移
        Revolve_set2468(1,1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position==5)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(712,-712);//右移
        delay_ms(10);
        set_angle(110,110);//上移
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position==6)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(990,-990);//右移
        delay_ms(10);
        set_angle(110,110);//上移
        Revolve_set2468(-1,-1,-1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==3&&position>=7&&position<=9)
    {
        set_angle(-970,-970);//下移
        Absorb();
        set_angle(435+(position-7)*277,-435-(position-7)*277);//右移
        delay_ms(10);
        set_angle(-167,-167);//下移
        if(position==9)
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        else if(position==7)
        {
            Revolve_set1379(1,1,-1,1);
        }
        else if(position==8)
        {
            Revolve_set2468(-1,1,1,1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_black_2(void)
{
    if(color==0&&num==2&&position==1)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(430,-430);//右移
        delay_ms(10);
        set_angle(150,150);//上移
        Revolve_set1379(1,-1,1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==2)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(707,-707);//右移
        delay_ms(10);
        set_angle(150,150);//上移
        Revolve_set2468(1,-1,-1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==3)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(984,-984);//右移
        delay_ms(10);
        set_angle(150,150);//上移
        Revolve_set1379(-1,-1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==4)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(430,-430);//右移
        delay_ms(10);
        set_angle(-120,-120);//下移
        Revolve_set2468(1,1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==5)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(720,-720);//右移
        delay_ms(10);
        set_angle(-120,-120);//下移
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==6)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(997,-997);//右移
        delay_ms(10);
        set_angle(-120,-120);//下移
        Revolve_set2468(-1,-1,-1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==7)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(430,-430);//右移
        delay_ms(10);
        set_angle(-397,-397);//下移
        Revolve_set1379(1,1,-1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==8)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(710,-710);//右移
        delay_ms(10);
        set_angle(-397,-397);//下移
        Revolve_set2468(-1,1,1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==2&&position==9)
    {
        set_angle(-740,-740);//下移
        Absorb();
        set_angle(997,-997);//右移
        delay_ms(10);
        set_angle(-397,-397);//下移
        if(flag_3) //顺时针旋转为例
        {
            Revolve_set1379(-1,1,-1,-1);
        }
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
void ONE_black_1(void)
{
    if(color==0&&num==1&&position==1)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(430,-430);//右移
        delay_ms(10);
        set_angle(-45,-45);//下移
        Revolve_set1379(1,-1,1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
        
        
    }
    else if(color==0&&num==1&&position==2)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(707,-707);//右移
        delay_ms(10);
        set_angle(-45,-45);//下移
        Revolve_set2468(1,-1,-1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==3)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(984,-984);//右移
        delay_ms(10);
        set_angle(-45,-45);//下移
        Revolve_set1379(-1,-1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==4)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(440,-440);//右移
        delay_ms(10);
        set_angle(-322,-322);//下移
        Revolve_set2468(1,1,1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==5)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(717,-717);//右移
        delay_ms(10);
        set_angle(-312,-312);//下移
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==6)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(984,-984);//右移
        delay_ms(10);
        set_angle(-322,-322);//下移
        Revolve_set2468(-1,-1,-1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==7)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(440,-440);//右移
        delay_ms(10);
        set_angle(-600,-600);//下移
        Revolve_set1379(1,1,-1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==8)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(717,-717);//右移
        delay_ms(10);
        set_angle(-600,-600);//下移
        Revolve_set2468(-1,1,1,1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
    else if(color==0&&num==1&&position==9)
    {
        set_angle(-544,-544);
        Absorb();
        set_angle(994,-994);//右移
        delay_ms(10);
        set_angle(-600,-600);//下移
        Revolve_set1379(-1,1,-1,-1);
        Laydown();
        set_angle((int)(-(g_stepper1.add_pulse_count*MAX_STEP_ANGLE)),(int)(-(g_stepper4.add_pulse_count*MAX_STEP_ANGLE)));
        start=0;
    }
}
