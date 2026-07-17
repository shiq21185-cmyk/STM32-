#ifndef __HC06_H
#define __HC06_H

#include "stm32f10x.h"

// ==================== ���ݽṹ���� ====================

/**
  * @brief  ʱ�����ݽṹ
  * @note   ���ڴ洢���������յ���ϵͳʱ��
  */
typedef struct {
    uint8_t hour;      // Сʱ (0-23)
    uint8_t minute;    // ���� (0-59)
    uint8_t second;    // �� (0-59)
} HC06_Time_t;

/**
  * @brief  �������ݽṹ
  * @note   ���ڴ洢���ӵ�ʱ������
  */
typedef struct {
    uint8_t hour;      // ����Сʱ (0-23)
    uint8_t minute;    // ���ӷ��� (0-59)
    uint8_t enabled;   // ����ʹ�ܱ�־ (1=����, 0=����)
} HC06_Alarm_t;

// ==================== ȫ�ֱ������� ====================

extern volatile HC06_Time_t g_bt_time;         // ���������յ�������ʱ��
extern volatile uint8_t g_bt_time_valid;       // ����ʱ���Ƿ���Ч��־
extern volatile uint8_t g_bt_time_updated;     // ����ʱ���Ƿ���±�־
extern volatile uint8_t g_bt_debug_buf[3];     // �������Ի��������洢���3���ֽ�
extern volatile uint8_t g_bt_debug_ready;      // �������ݾ�����־
extern volatile uint8_t g_bt_rx_count;         // ���������ֽڼ���

extern volatile HC06_Alarm_t g_bt_alarms[3];   // 3��������������
extern volatile uint8_t g_bt_alarm_updated;    // �����Ƿ���±�־
extern volatile uint8_t g_bt_alarm_index;      // ���¸��µ���������

// ==================== �������� ====================

/**
  * @brief  HC06����ģ���ʼ��
  * @param  ��
  * @retval ��
  * @note   ��ʼ������ȫ�ֱ�����Ĭ����������
  */
void HC06_Init(void);

/**
  * @brief  �������յ��ĵ����ֽ�����
  * @param  byte: ���յ����ֽ�
  * @retval ��
  * @note   ͨ��״̬������3�ֽ�Э�飬��ʱʱ��500ms
  *         Э���ʽ��
  *         - ʱ��ͬ����0-23��Сʱ��, 0-59�����ӣ�, 0-59���룩
  *         - �������ã�24-26������1-3��, 0-23��Сʱ��, 0-59�����ӣ�
  */
void HC06_ProcessByte(uint8_t byte);

/**
  * @brief  ��ȡ������ͬ����ʱ��
  * @param  time: ����������洢ʱ���ָ��
  * @retval 1=�ɹ���ȡ, 0=����Чʱ��
  * @note   ��ȡ���������±�־
  */
uint8_t HC06_GetTime(HC06_Time_t *time);

/**
  * @brief  ����Ƿ����µ�����ʱ������
  * @param  ��
  * @retval 1=��������, 0=��������
  * @note   ��ȡ�󲻻������־�������HC06_ClearTimeFlag���
  */
uint8_t HC06_HasNewTime(void);

/**
  * @brief  �������ʱ����±�־
  * @param  ��
  * @retval ��
  */
void HC06_ClearTimeFlag(void);

/**
  * @brief  ����Ƿ����µ���������
  * @param  ��
  * @retval 1=��������, 0=��������
  */
uint8_t HC06_HasNewAlarm(void);

/**
  * @brief  ��ȡ���¸��µ���������
  * @param  index: ����������������� (0-2)
  * @param  alarm: �����������������ָ��
  * @retval 1=�ɹ���ȡ, 0=��������
  * @note   ��ȡ���������±�־
  */
uint8_t HC06_GetNewAlarm(uint8_t *index, HC06_Alarm_t *alarm);

/**
  * @brief  ����ָ�����ӵ�ʱ��
  * @param  index: �������� (0-2)
  * @param  hour: ����Сʱ (0-23)
  * @param  minute: ���ӷ��� (0-59)
  * @param  enabled: ����ʹ�ܱ�־
  * @retval ��
  * @note   �����ø��±�־
  */
void HC06_SetAlarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled);

/* Save the three alarm settings in internal Flash. */
void HC06_SaveAlarms(void);

#endif
