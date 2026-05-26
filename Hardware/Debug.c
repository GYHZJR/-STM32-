#include "Debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void Debug_Init(uint32_t baud) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // PA9=TX, push-pull AF
    GPIO_InitTypeDef g;
    g.GPIO_Pin = GPIO_Pin_9;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &g);

    USART_InitTypeDef u;
    u.USART_BaudRate = baud;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode = USART_Mode_Tx;
    u.USART_Parity = USART_Parity_No;
    u.USART_StopBits = USART_StopBits_1;
    u.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &u);

    USART_Cmd(USART1, ENABLE);
}

void Debug_SendByte(uint8_t byte) {
    while (!USART_GetFlagStatus(USART1, USART_FLAG_TXE));
    USART_SendData(USART1, byte);
}

void Debug_SendString(const char *str) {
    while (*str) Debug_SendByte((uint8_t)*str++);
}

void Debug_SendNum(int32_t num) {
    char buf[12];
    int i = 0, neg = 0;
    if (num < 0) { neg = 1; num = -num; }
    do { buf[i++] = '0' + (num % 10); num /= 10; } while (num);
    if (neg) buf[i++] = '-';
    while (i--) Debug_SendByte((uint8_t)buf[i]);
}

void Debug_SendFloat(float num, uint8_t decimals) {
    if (num < 0) { Debug_SendByte('-'); num = -num; }
    int32_t intPart = (int32_t)num;
    Debug_SendNum(intPart);
    Debug_SendByte('.');
    float frac = num - intPart;
    while (decimals--) {
        frac *= 10;
        Debug_SendByte('0' + ((uint8_t)frac % 10));
    }
}

void Debug_Printf(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Debug_SendString(buf);
}

void Debug_HexDump(const char *label, uint8_t *data, uint16_t len) {
    Debug_SendString(label);
    Debug_SendString(": ");
    uint16_t i;
    for (i = 0; i < len; i++) {
        uint8_t hi = (data[i] >> 4) & 0x0F;
        uint8_t lo = data[i] & 0x0F;
        Debug_SendByte(hi < 10 ? '0' + hi : 'A' + hi - 10);
        Debug_SendByte(lo < 10 ? '0' + lo : 'A' + lo - 10);
        Debug_SendByte(' ');
    }
    Debug_SendString("\r\n");
}

/* ---------- printf redirect for standard library ---------- */
#ifdef __MICROLIB
int fputc(int ch, FILE *f) {
    Debug_SendByte((uint8_t)ch);
    return ch;
}
#endif
