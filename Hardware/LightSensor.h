#ifndef __LIGHTSENSOR_H
#define __LIGHTSENSOR_H
#include "stm32f10x.h"

void LightSensor_Init(void);
uint16_t LightSensor_Read(void);    // 0~4095
uint8_t LightSensor_GetPercent(void); // 0~100%

#endif
