#ifndef __MPU6050_H
#define __MPU6050_H
#include "stm32f10x.h"

#define MPU6050_ADDR  0xD0  // AD0=0 → 0x68 << 1

void MPU6050_Init(void);
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);
void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz);

#endif
