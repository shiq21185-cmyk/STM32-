#include "stm32f10x.h"
#include "dht11.h"
#include "Delay.h"

/* DHT11 最近一次校验成功的湿度和温度数据。 */
unsigned int rec_data[4] = {0};

/* 将 PA5 配置为推挽输出，用于发送 DHT11 起始信号。 */
void DH11_GPIO_Init_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/* 将 PA5 配置为浮空输入，用于读取 DHT11 返回数据。 */
void DH11_GPIO_Init_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/* 发送低电平起始信号，并切换为输入等待传感器响应。 */
void DHT11_Start(void)
{
    DH11_GPIO_Init_OUT();
    dht11_high;
    Delay_us(30);
    dht11_low;
    Delay_ms(20);
    dht11_high;
    Delay_us(30);
    DH11_GPIO_Init_IN();
}

/* 按 DHT11 时序读取一个字节：40 us 后仍为高电平表示逻辑 1。 */
char DHT11_Rec_Byte(void)
{
    unsigned char i = 0;
    unsigned char data = 0;

    for(i = 0; i < 8; i++)
    {
        while(Read_Data == 0);
        Delay_us(40);
        data <<= 1;
        if(Read_Data == 1)
        {
            data |= 1;
        }
        while(Read_Data == 1);
    }
    return data;
}

/* 读取 DHT11 的 5 字节帧，并在校验和正确时更新测量结果。 */
void DHT11_REC_Data(void)
{
    unsigned char R_H, R_L, T_H, T_L;
    unsigned char CHECK;

    DHT11_Start();
    if(Read_Data == 0)
    {
        while(Read_Data == 0);
        while(Read_Data == 1);

        R_H = DHT11_Rec_Byte();
        R_L = DHT11_Rec_Byte();
        T_H = DHT11_Rec_Byte();
        T_L = DHT11_Rec_Byte();
        CHECK = DHT11_Rec_Byte();

        dht11_low;
        Delay_us(55);
        dht11_high;

        if((R_H + R_L + T_H + T_L) == CHECK)
        {
            rec_data[0] = R_H;
            rec_data[1] = R_L;
            rec_data[2] = T_H;
            rec_data[3] = T_L;
        }
    }
}
