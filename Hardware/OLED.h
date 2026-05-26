#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x.h"

#define OLED_ADDR  0x78  // SSD1306 I2C address (0x3C << 1)

void I2C_BusRecovery(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t intLen, uint8_t decLen);

#endif
