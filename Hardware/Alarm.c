#include "Alarm.h"
#include "Delay.h"

#define ABS(v) ((v) < 0 ? -(v) : (v))

AlarmCfg g_Alarm;

void Alarm_Init(void) {
    g_Alarm.tempHigh = 40.0f;
    g_Alarm.tempLow = -10.0f;
    g_Alarm.accelThresh = 0;
    g_Alarm.alarmFlags = 0;
    g_Alarm.lastAlarmTick = 0;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef g = {GPIO_Pin_0, GPIO_Speed_50MHz, GPIO_Mode_Out_PP};
    GPIO_Init(GPIOB, &g); // buzzer on PB0 (active LOW)
    GPIO_SetBits(GPIOB, GPIO_Pin_0);  // HIGH = off

    // LEDs
    GPIO_InitTypeDef l = {GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4, GPIO_Speed_50MHz, GPIO_Mode_Out_PP};
    GPIO_Init(GPIOB, &l);
    GPIO_ResetBits(GPIOB, GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_4);
}

void Alarm_Check(float temp, int16_t ax, int16_t ay, int16_t az) {
    uint8_t flags = 0;

    if (temp > g_Alarm.tempHigh) flags |= 0x01;   // high temp
    if (temp < g_Alarm.tempLow)  flags |= 0x02;   // low temp

    // tilt: any axis deviates >2500 (~0.15g, ~9°) from rest (0,0,16384)
    if (ABS(ax) > 2500 || ABS(ay) > 2500 || ABS((int32_t)az - 16384) > 2500)
        flags |= 0x04;

    if (flags) {
        g_Alarm.alarmFlags = flags;
        GPIO_ResetBits(GPIOB, GPIO_Pin_0); // LOW = buzzer on
    } else {
        g_Alarm.alarmFlags = 0;
        GPIO_SetBits(GPIOB, GPIO_Pin_0);   // HIGH = buzzer off
    }

    // Update LEDs
    if (flags & 0x01) GPIO_SetBits(GPIOB, GPIO_Pin_1);   // red LED
    else              GPIO_ResetBits(GPIOB, GPIO_Pin_1);

    if (flags & 0x02) GPIO_SetBits(GPIOB, GPIO_Pin_4);   // yellow LED
    else              GPIO_ResetBits(GPIOB, GPIO_Pin_4);

    if (flags & 0x04) GPIO_SetBits(GPIOB, GPIO_Pin_3);   // blue LED
    else              GPIO_ResetBits(GPIOB, GPIO_Pin_3);
}

void Alarm_Trigger(uint8_t flag) {
    g_Alarm.alarmFlags = flag;
    if (flag) GPIO_ResetBits(GPIOB, GPIO_Pin_0);  // LOW = on
    else      GPIO_SetBits(GPIOB, GPIO_Pin_0);    // HIGH = off
}

void Alarm_Clear(void) {
    g_Alarm.alarmFlags = 0;
    GPIO_SetBits(GPIOB, GPIO_Pin_0);  // HIGH = off
}
