#include "stm32f10x.h"                  // Device header
void LightSenor_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

	GPIO_InitTypeDef GPIO_InStructure;
	GPIO_InStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InStructure);
}

uint8_t LightSenor_Get(void){
	return GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4);
}
