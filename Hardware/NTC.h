#ifndef __NTC_H
#define __NTC_H
#include "stm32f10x.h"

void NTC_Init(void);
float NTC_ReadTemp(void);    // return temperature in °C

#endif
