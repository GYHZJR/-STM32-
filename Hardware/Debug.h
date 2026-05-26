#ifndef __DEBUG_H
#define __DEBUG_H
#include "stm32f10x.h"

void Debug_Init(uint32_t baud);          // init USART1 with given baud rate
void Debug_SendByte(uint8_t byte);
void Debug_SendString(const char *str);
void Debug_SendNum(int32_t num);
void Debug_SendFloat(float num, uint8_t decimals);
void Debug_Printf(const char *fmt, ...); // lightweight printf-like
void Debug_HexDump(const char *label, uint8_t *data, uint16_t len);

#endif
