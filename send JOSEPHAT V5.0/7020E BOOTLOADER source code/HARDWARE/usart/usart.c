/*
 * usart.c
 *
 *  Created on: 2017年5月6日
 *      Author: xianlee
 */
#include "stm32f0xx.h"
#include "stdarg.h"
#include "led.h"
#include "sim900a.h"

extern bool IsOpen;

void usart1_init(u32 bound) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);



    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);


    NVIC_InitStruct.NVIC_IRQChannel = USART3_6_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0x02;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

void NVIC_Configuration_Uart2( void )
{
//	NVIC_InitTypeDef NVIC_InitStructure;

//	/* Enable the USART2 Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);

}

void tim7_init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); //TIM7时钟使能

    //TIM7初始化设置
    TIM_TimeBaseStructure.TIM_Period = 5000-1;//5ms
    TIM_TimeBaseStructure.TIM_Prescaler = 48-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);

    TIM_ITConfig( TIM7, TIM_IT_Update, ENABLE );

    //TIM7中断分组配置

	  NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPriority=3;
		NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_ClearITPendingBit(TIM7, TIM_IT_Update); //清除中断标志
    TIM_Cmd(TIM7, DISABLE); //停止定时器

}
//UART2 GPRS
void usart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStruct;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);



    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);


    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0x02;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

char *
itoa(n, base)
long n;   /* abs k16 */
int base;
{
    register char *p;
    register int minus;
    static char buf[36];
    p = &buf[36];
    *--p = '/0';
    if (n < 0) {
        minus = 1;
        n = -n;
    }
    else
        minus = 0;
    if (n == 0)
        *--p = '0';
    else
        while (n > 0) {
            *--p = "0123456789abcdef"[n % base];
            n /= base;
        }
    if (minus)
        *--p = '-';
    return p;
}

void usart_printf(USART_TypeDef* USARTx, char *Data, ...) {
    const char *s;
    int d;
    char buf[16];

    va_list ap;
    va_start(ap, Data);

    while (*Data != 0) {
        if (*Data == 0x5c) {
            switch (*++Data) {
            case 'r':
                USART_SendData(USARTx, 0x0d);
                Data++;
                break;

            case 'n':
                USART_SendData(USARTx, 0x0a);
                Data++;
                break;

            default:
                Data++;
                break;
            }
        } else if (*Data == '%') {
            switch (*++Data) {
            case 's':
                s = va_arg(ap, const char *);
                for (; *s; s++) {
                    USART_SendData(USARTx, *s);
                    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
                        ;
                }
                Data++;
                break;

            case 'd':
                d = va_arg(ap, int);
                itoa(d, buf, 10);
                for (s = buf; *s; s++) {
                    USART_SendData(USARTx, *s);
                    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
                        ;
                }
                Data++;
                break;
            default:
                Data++;
                break;
            }
        } else
            USART_SendData(USARTx, *Data++);
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
            ;
    }
}


/**
  * @brief  This function handles USART1 Handler.
  * @param  None
  * @retval None
  */
void USART3_6_IRQHandler(void) {
    char ch;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        ch = USART_ReceiveData(USART3);
//    if(ch == 'A')
//		{
//      IsOpen = true;
//		}
        USART_SendData(USART3, ch);
        while( USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }

    if (USART_GetITStatus(USART3, USART_IT_IDLE) == SET) {
        USART_ClearFlag(USART3, USART_FLAG_ORE);
        USART_ReceiveData(USART3);
        ch = USART_ReceiveData(USART3);
    }
}

uint8_t flbus;
void  TIM7_IRQHandler(void)
{
  if(TIM_GetITStatus(TIM7,TIM_IT_Update)!=RESET)
  {
    TIM_ClearITPendingBit(TIM7,TIM_IT_Update);//清除中断标志
    TIM_Cmd(TIM7,DISABLE);//停止定时器
    if(flbus==0x01)
    {
      end_frame=1;//置位帧结束标记
      flbus=0x00;
    }
  }
}



void USART2_IRQHandler(void)
{
    char ch;

    //读数据寄存器非空
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
//		USART_ClearFlag(USART3,USART_IT_RXNE);
        ch = USART_ReceiveData(USART2);
        //预留1个字节写结束符
        if (gsm_global_data.frame_len < (GSM_DATA_RECORD_MAX_LEN - 1)) {
            
            TIM_ClearITPendingBit(TIM7,TIM_IT_Update);//清除定时器溢出中断
            gsm_global_data.frame_buf[gsm_global_data.frame_len++] = ch;

            TIM_SetCounter(TIM7,0);//当接收到一个新的字节，将定时器7复位为0，重新计时（相当于喂狗）
            TIM_Cmd(TIM7,ENABLE);//开
            flbus=1;
        }
//#ifdef AT_DEBUG
//		/* 开启DEBUG会影响HTTP数据接收 */
//		USART_SendData(USART2, ch);
//		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//#endif
    }
}

//******************************************************************************
// Hosting of stdio functionality through USART2
//******************************************************************************

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE {
    int handle; /* Add whatever you need here */
};
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
    static int last;

    if ((ch == (int)'\n') && (last != (int)'\r')) {
        last = (int)'\r';

        while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;
        USART_SendData(DEBUG_USART, last);
    } else {
        last = ch;
    }

    while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
        ;

    USART_SendData(DEBUG_USART, ch);

    return (ch);
}

int fgetc(FILE *f)
{
    char ch;

    while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_RXNE) == RESET);

    ch = USART_ReceiveData(DEBUG_USART);

    return((int)ch);
}

int ferror(FILE *f)
{
    /* Your implementation of ferror */
    return EOF;
}

void _ttywrch(int ch)
{
    static int last;

    if ((ch == (int)'\n') && (last != (int)'\r')) {
        last = (int)'\r';

        while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
            ;

        USART_SendData(DEBUG_USART, last);
    } else {
        last = ch;
    }

    while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET)
        ;

    USART_SendData(DEBUG_USART, ch);
}

void _sys_exit(int return_code)
{
label:
    goto label;  /* endless loop */
}
