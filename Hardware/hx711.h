#ifndef __HX711_H
#define __HX711_H

#include "stm32f10x.h"
#include "stdlib.h"

/* HX711 数据接口：SCK 使用 PB14，DOUT 使用 PB15。 */
#define HX711_SCK_HIGH()  GPIO_SetBits(GPIOB, GPIO_Pin_14)
#define HX711_SCK_LOW()   GPIO_ResetBits(GPIOB, GPIO_Pin_14)
#define HX711_DOUT_READ() GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15)

void Init_HX711pin(void);
u32 HX711_Read(void);
void Get_Maopi(void);
void Get_Weight(void);

extern u32 HX711_Buffer;
extern u32 Weight_Maopi;
extern s32 Weight_Shiwu;
extern u8 Flag_Error;

#endif
