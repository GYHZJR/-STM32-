#ifndef __ALARM_H
#define __ALARM_H
#include "stm32f10x.h"

typedef struct {
    float tempHigh;
    float tempLow;
    float accelThresh;
    uint8_t alarmFlags; // bit0=highTemp, bit1=lowTemp, bit2=tilt
    uint32_t lastAlarmTick;
} AlarmCfg;

extern AlarmCfg g_Alarm;

void Alarm_Init(void);
void Alarm_Check(float temp, int16_t ax, int16_t ay, int16_t az);
void Alarm_Trigger(uint8_t flag);
void Alarm_Clear(void);

#endif
