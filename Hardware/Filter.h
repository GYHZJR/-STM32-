#ifndef __FILTER_H
#define __FILTER_H
#include "stm32f10x.h"

#define FILTER_SIZE  3

void Filter_Init(void);
float Filter_Add(float newValue);
float Filter_Get(void);

#endif
