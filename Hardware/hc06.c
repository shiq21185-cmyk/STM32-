#include "hc06.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include "app_tasks.h"
#include "stm32f10x_flash.h"

/* Last 1 KB page of STM32F103C8 Flash, reserved for alarm settings. */
#define ALARM_FLASH_ADDRESS       0x0800FC00UL
#define ALARM_FLASH_MAGIC         0xA65AU
#define ALARM_FLASH_WORD_COUNT    5U
// ==================== ȫ�ֱ������� ====================

/**
  * @brief  ���������յ�������ʱ��
  */
volatile HC06_Time_t g_bt_time = {0};

/**
  * @brief  ����ʱ���Ƿ���Ч��־
  * @note   1=��Ч, 0=��Ч
  */
volatile uint8_t g_bt_time_valid = 0;

/**
  * @brief  ����ʱ���Ƿ���±�־
  * @note   1=��������, 0=��������
  */
volatile uint8_t g_bt_time_updated = 0;

/**
  * @brief  �������Ի�����
  * @note   �洢���3�����յ����ֽڣ����ڵ���
  */
volatile uint8_t g_bt_debug_buf[3] = {0};

/**
  * @brief  �������ݾ�����־
  * @note   1=��������, 0=��������
  */
volatile uint8_t g_bt_debug_ready = 0;

/**
  * @brief  ���������ֽڼ���
  * @note   �ۼ�ͳ�ƽ��յ����ֽ���
  */
volatile uint8_t g_bt_rx_count = 0;

/**
  * @brief  3��������������
  * @note   ����0-2��Ӧ����1-3
  */
volatile HC06_Alarm_t g_bt_alarms[3] = {
    {ALARM_HOUR_1, ALARM_MINUTE_1, 1},    // ����1Ĭ��ֵ
    {ALARM_HOUR_2, ALARM_MINUTE_2, 1},    // ����2Ĭ��ֵ
    {ALARM_HOUR_3, ALARM_MINUTE_3, 1}     // ����3Ĭ��ֵ
};

/**
  * @brief  �����Ƿ���±�־
  * @note   1=��������, 0=��������
  */
volatile uint8_t g_bt_alarm_updated = 0;

/**
  * @brief  ���¸��µ���������
  * @note   0-2��Ӧ����1-3
  */
volatile uint8_t g_bt_alarm_index = 0;

static uint16_t Alarm_Pack(const volatile HC06_Alarm_t *alarm)
{
    return (uint16_t)alarm->hour |
           ((uint16_t)alarm->minute << 8) |
           ((uint16_t)(alarm->enabled ? 1U : 0U) << 15);
}

static uint8_t Alarm_DataValid(uint16_t word)
{
    uint8_t hour = (uint8_t)(word & 0xFFU);
    uint8_t minute = (uint8_t)((word >> 8) & 0x7FU);
    return (hour <= 23U && minute <= 59U);
}

static void HC06_LoadAlarms(void)
{
    const volatile uint16_t *flash = (const volatile uint16_t *)ALARM_FLASH_ADDRESS;
    uint16_t checksum;
    uint8_t i;

    checksum = (uint16_t)(ALARM_FLASH_MAGIC ^ flash[1] ^ flash[2] ^ flash[3] ^ 0x5AA5U);
    if(flash[0] != ALARM_FLASH_MAGIC || flash[4] != checksum)
    {
        return;
    }

    for(i = 0; i < 3U; i++)
    {
        if(!Alarm_DataValid(flash[i + 1U]))
        {
            return;
        }
    }

    for(i = 0; i < 3U; i++)
    {
        uint16_t word = flash[i + 1U];
        g_bt_alarms[i].hour = (uint8_t)(word & 0xFFU);
        g_bt_alarms[i].minute = (uint8_t)((word >> 8) & 0x7FU);
        g_bt_alarms[i].enabled = (uint8_t)((word >> 15) & 0x01U);
    }
}

void HC06_SaveAlarms(void)
{
    uint16_t words[ALARM_FLASH_WORD_COUNT];
    uint8_t i;

    taskENTER_CRITICAL();
    words[0] = ALARM_FLASH_MAGIC;
    words[1] = Alarm_Pack(&g_bt_alarms[0]);
    words[2] = Alarm_Pack(&g_bt_alarms[1]);
    words[3] = Alarm_Pack(&g_bt_alarms[2]);
    words[4] = (uint16_t)(words[0] ^ words[1] ^ words[2] ^ words[3] ^ 0x5AA5U);

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    if(FLASH_ErasePage(ALARM_FLASH_ADDRESS) == FLASH_COMPLETE)
    {
        for(i = 0; i < ALARM_FLASH_WORD_COUNT; i++)
        {
            if(FLASH_ProgramHalfWord(ALARM_FLASH_ADDRESS + (uint32_t)i * 2U, words[i]) != FLASH_COMPLETE)
            {
                break;
            }
        }
    }
    FLASH_Lock();
    taskEXIT_CRITICAL();
}

// ==================== ���ĺ���ʵ�� ====================

/**
  * @brief  �������յ��ĵ����ֽ�����
  * @param  byte: ���յ����ֽ�
  * @retval ��
  * @note   ͨ��״̬������3�ֽ�Э�飺
  *         - ״̬0�����յ�1�ֽ�
  *         - ״̬1�����յ�2�ֽ�
  *         - ״̬2�����յ�3�ֽڲ�����
  * 
  *         Э���ʽ��
  *         - ʱ��ͬ������1�ֽ�0-23��Сʱ��, ��2�ֽ�0-59�����ӣ�, ��3�ֽ�0-59���룩
  *         - �������ã���1�ֽ�24-26������1-3��, ��2�ֽ�0-23��Сʱ��, ��3�ֽ�0-59�����ӣ�
  * 
  *         ��ʱ���ƣ�����500msδ���յ���һ�ֽ�������״̬��
  *         ���⴦��������13:10��ͷ�Ĵ���ʱ�䣨�����Ͽ�ʱ��Ĭ��ֵ��
  */
void HC06_ProcessByte(uint8_t byte)
{
    static uint8_t stage = 0;              // ����״̬��״̬ (0-2)
    static uint8_t temp_buf[3];            // ��ʱ���������洢3���ֽ�
    static uint32_t last_byte_tick = 0;    // ��һ�ֽڵĽ���ʱ��
    uint32_t now = xTaskGetTickCount();    // ��ǰʱ��
    
    uint8_t first, second, third;          // �����ֽڵ���ʱ����
    uint8_t alarm_index;                   // ��������
    
    // ��ʱ��⣺����500msδ���յ���һ�ֽ�������״̬��
    if((now - last_byte_tick) > pdMS_TO_TICKS(500) && stage != 0) {
        stage = 0;
    }
    last_byte_tick = now;                  // ������һ�ֽ�ʱ��
    
    // ���µ��Ի������ͼ���
    g_bt_debug_buf[g_bt_rx_count % 3] = byte;
    g_bt_rx_count++;
    
    // ״̬������
    switch(stage) {
        case 0:  // ���յ�1�ֽ�
            temp_buf[0] = byte;
            stage = 1;
            break;
            
        case 1:  // ���յ�2�ֽ�
            temp_buf[1] = byte;
            stage = 2;
            break;
            
        case 2:  // ���յ�3�ֽڲ�����
            temp_buf[2] = byte;
            stage = 0;  // ����״̬��
            
            first = temp_buf[0];
            second = temp_buf[1];
            third = temp_buf[2];
            
            // ���µ��Ի�����
            g_bt_debug_buf[0] = first;
            g_bt_debug_buf[1] = second;
            g_bt_debug_buf[2] = third;
            g_bt_debug_ready = 1;
            
            // �ж����������û���ʱ��ͬ��
            if(first >= 24) {
                // �������ã���1�ֽ�24-26��Ӧ����1-3
                alarm_index = first - 24;
                
                // ��֤������Ч�ԣ���������<3��Сʱ<=23������<=59
                if(alarm_index < 3 && second <= 23 && third <= 59) {
                    taskENTER_CRITICAL();
                    g_bt_alarms[alarm_index].hour = second;
                    g_bt_alarms[alarm_index].minute = third;
                    g_bt_alarms[alarm_index].enabled = 1;
                    g_bt_alarm_updated = 1;
                    g_bt_alarm_index = alarm_index;
                    taskEXIT_CRITICAL();
                    HC06_SaveAlarms();
                }
            } else {
                // ʱ��ͬ������1�ֽ�0-23��Сʱ��
                if(first <= 23 && second <= 59 && third <= 59) {
                    taskENTER_CRITICAL();
                    // ���������Ͽ�ʱ������13:10��ͷ�Ĵ���ʱ��
                    if(!(first == 13 && second == 10)) {
                        g_bt_time.hour = first;
                        g_bt_time.minute = second;
                        g_bt_time.second = third;
                        g_bt_time_valid = 1;
                        g_bt_time_updated = 1;
                    }
                    taskEXIT_CRITICAL();
                }
            }
            break;
            
        default:  // �쳣״̬������
            stage = 0;
            break;
    }
}

/**
  * @brief  ��ȡ������ͬ����ʱ��
  * @param  time: ����������洢ʱ���ָ��
  * @retval 1=�ɹ���ȡ, 0=����Чʱ��
  * @note   ��ȡ���������±�־����Ҫʹ���ٽ�������
  */
uint8_t HC06_GetTime(HC06_Time_t *time)
{
    taskENTER_CRITICAL();
    if(g_bt_time_valid) {
        time->hour = g_bt_time.hour;
        time->minute = g_bt_time.minute;
        time->second = g_bt_time.second;
        g_bt_time_updated = 0;  // ������±�־
        taskEXIT_CRITICAL();
        return 1;
    }
    taskEXIT_CRITICAL();
    return 0;
}

/**
  * @brief  ����Ƿ����µ�����ʱ������
  * @param  ��
  * @retval 1=��������, 0=��������
  * @note   ��ȡ�󲻻������־�������HC06_ClearTimeFlag���
  */
uint8_t HC06_HasNewTime(void)
{
    uint8_t has_new;
    taskENTER_CRITICAL();
    has_new = g_bt_time_updated;
    if(has_new) {
        g_bt_time_updated = 0;  // �����־
    }
    taskEXIT_CRITICAL();
    return has_new;
}

/**
  * @brief  �������ʱ����±�־
  * @param  ��
  * @retval ��
  */
void HC06_ClearTimeFlag(void)
{
    taskENTER_CRITICAL();
    g_bt_time_updated = 0;
    taskEXIT_CRITICAL();
}

/**
  * @brief  ����Ƿ����µ���������
  * @param  ��
  * @retval 1=��������, 0=��������
  */
uint8_t HC06_HasNewAlarm(void)
{
    uint8_t has_new;
    taskENTER_CRITICAL();
    has_new = g_bt_alarm_updated;
    taskEXIT_CRITICAL();
    return has_new;
}

/**
  * @brief  ��ȡ���¸��µ���������
  * @param  index: ����������������� (0-2)
  * @param  alarm: �����������������ָ��
  * @retval 1=�ɹ���ȡ, 0=��������
  * @note   ��ȡ���������±�־
  */
uint8_t HC06_GetNewAlarm(uint8_t *index, HC06_Alarm_t *alarm)
{
    taskENTER_CRITICAL();
    if(g_bt_alarm_updated) {
        *index = g_bt_alarm_index;
        alarm->hour = g_bt_alarms[*index].hour;
        alarm->minute = g_bt_alarms[*index].minute;
        alarm->enabled = g_bt_alarms[*index].enabled;
        g_bt_alarm_updated = 0;  // ������±�־
        taskEXIT_CRITICAL();
        return 1;
    }
    taskEXIT_CRITICAL();
    return 0;
}

/**
  * @brief  ����ָ�����ӵ�ʱ��
  * @param  index: �������� (0-2)
  * @param  hour: ����Сʱ (0-23)
  * @param  minute: ���ӷ��� (0-59)
  * @param  enabled: ����ʹ�ܱ�־
  * @retval ��
  * @note   �����ø��±�־����Ҫʹ���ٽ�������
  */
void HC06_SetAlarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled)
{
    if(index >= 3) return;  // ����У��
    taskENTER_CRITICAL();
    g_bt_alarms[index].hour = hour;
    g_bt_alarms[index].minute = minute;
    g_bt_alarms[index].enabled = enabled;
    g_bt_alarm_updated = 1;
    g_bt_alarm_index = index;
    taskEXIT_CRITICAL();
    HC06_SaveAlarms();
}

/**
  * @brief  HC06����ģ���ʼ��
  * @param  ��
  * @retval ��
  * @note   ��ʼ������ȫ�ֱ�����Ĭ����������
  */
void HC06_Init(void)
{
    // ��ʼ��ʱ����ر���
    g_bt_time_valid = 0;
    g_bt_time_updated = 0;
    g_bt_rx_count = 0;
    g_bt_debug_ready = 0;
    
    // ��ʼ��������ر���
    g_bt_alarm_updated = 0;
    g_bt_alarm_index = 0;
    
    // ����Ĭ������ʱ��
    g_bt_alarms[0].hour = ALARM_HOUR_1;
    g_bt_alarms[0].minute = ALARM_MINUTE_1;
    g_bt_alarms[0].enabled = 1;
    
    g_bt_alarms[1].hour = ALARM_HOUR_2;
    g_bt_alarms[1].minute = ALARM_MINUTE_2;
    g_bt_alarms[1].enabled = 1;
    
    g_bt_alarms[2].hour = ALARM_HOUR_3;
    g_bt_alarms[2].minute = ALARM_MINUTE_3;
    g_bt_alarms[2].enabled = 1;

    /* Restore the last valid alarm configuration after a power cycle. */
    HC06_LoadAlarms();
}
