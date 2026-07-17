#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"
#include <stdint.h>

/* 四个按键：切换页面、编辑确认、减小、增大。 */
#define KEY1 0
#define KEY2 1
#define KEY3 2
#define KEY4 3
#define KEY_NUM 4

typedef enum { KEY_IDLE = 0, KEY_PRESS, KEY_RELEASE } KeyState_t;

typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
    KeyState_t state;
    uint32_t press_time;
    uint8_t press_flag;
} Key_t;

void Key_Init(void);
void Key_Scan(void);
uint8_t Key_GetPress(uint8_t key_num);

#endif
