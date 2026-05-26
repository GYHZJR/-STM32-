#ifndef __W25Q64_H
#define __W25Q64_H
#include "stm32f10x.h"

#define W25Q64_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define W25Q64_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

void W25Q64_Init(void);
void W25Q64_ReadID(uint16_t *id);
void W25Q64_SectorErase(uint32_t addr);
void W25Q64_PageWrite(uint32_t addr, uint8_t *data, uint16_t len);
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint16_t len);

#endif
