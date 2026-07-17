#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* DHT11 数据线接在 PA5。 */
#define dht11_high GPIO_SetBits(GPIOA, GPIO_Pin_5)
#define dht11_low GPIO_ResetBits(GPIOA, GPIO_Pin_5)
#define Read_Data GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5)

/* 湿度整数/小数与温度整数/小数，依次存放在 0 至 3 下标。 */
extern unsigned int rec_data[4];

void DH11_GPIO_Init_OUT(void);
void DH11_GPIO_Init_IN(void);
void DHT11_Start(void);
char DHT11_Rec_Byte(void);
void DHT11_REC_Data(void);

#endif
