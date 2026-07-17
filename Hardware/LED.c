#include "stm32f10x.h"                  // Device header


void LED_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef GPIO_InStructure;
	GPIO_InStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InStructure);
	/* PA0 为低电平点亮。临时调试：初始化后保持常亮。 */
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
	
}


void LED0_ON(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
}
void LED0_OFF(void)
{
	/* 临时保持常亮，忽略关灯请求。 */
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
}
void LED0_Turn(void)
{
	/* 临时保持常亮，忽略翻转请求。 */
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
}


