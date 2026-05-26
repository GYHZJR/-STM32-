#include "W25Q64.h"

// W25Q64 指令码
#define CMD_WREN         0x06  // 写使能
#define CMD_SECTOR_ERASE 0x20  // 扇区擦除（4KB）
#define CMD_PAGE_PROG    0x02  // 页编程（最多256字节）
#define CMD_READ_DATA    0x03  // 读数据
#define CMD_READ_ID      0x9F  // 读 JEDEC ID

// 通信失败一次后自动跳过后续操作，避免死等
static uint8_t w25q64_ok = 1;

// SPI 全双工：发一字节的同时收一字节
static uint8_t SPI_Transfer(uint8_t byte) {
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
    SPI_I2S_SendData(SPI1, byte);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

// 轮询 BUSY 位，超时则标记芯片不可用
static int W25Q64_WaitBusy(void) {
    uint32_t t = 500000;
    W25Q64_CS_LOW();
    SPI_Transfer(0x05);  // 读状态寄存器1
    while (SPI_Transfer(0xFF) & 0x01) {
        if (--t == 0) { W25Q64_CS_HIGH(); w25q64_ok = 0; return -1; }
    }
    W25Q64_CS_HIGH();
    return 0;
}

void W25Q64_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef g;
    g.GPIO_Speed = GPIO_Speed_50MHz;

    g.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_6;
    g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_4;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &g);
    W25Q64_CS_HIGH();

    SPI_InitTypeDef s;
    s.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode = SPI_Mode_Master;
    s.SPI_DataSize = SPI_DataSize_8b;
    s.SPI_CPOL = SPI_CPOL_High;
    s.SPI_CPHA = SPI_CPHA_2Edge;
    s.SPI_NSS = SPI_NSS_Soft;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    s.SPI_FirstBit = SPI_FirstBit_MSB;
    s.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &s);
    SPI_Cmd(SPI1, ENABLE);
}

void W25Q64_ReadID(uint16_t *id) {
    if (!w25q64_ok) { *id = 0xFFFF; return; }
    W25Q64_CS_LOW();
    SPI_Transfer(CMD_READ_ID);
    *id = SPI_Transfer(0xFF) << 8;
    *id |= SPI_Transfer(0xFF);
    W25Q64_CS_HIGH();
}

// 扇区擦除：Flash 写前必须擦（1→0 可写，0→1 靠擦除）
void W25Q64_SectorErase(uint32_t addr) {
    if (!w25q64_ok) return;
    W25Q64_CS_LOW();
    SPI_Transfer(CMD_WREN);  // 写使能
    W25Q64_CS_HIGH();

    W25Q64_CS_LOW();
    SPI_Transfer(CMD_SECTOR_ERASE);
    SPI_Transfer((uint8_t)(addr >> 16));
    SPI_Transfer((uint8_t)(addr >> 8));
    SPI_Transfer((uint8_t)(addr));
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();  // 擦除是异步的，等芯片完成
}

// 页编程：最大 256 字节，不可跨页
void W25Q64_PageWrite(uint32_t addr, uint8_t *data, uint16_t len) {
    if (!w25q64_ok) return;
    W25Q64_CS_LOW();
    SPI_Transfer(CMD_WREN);
    W25Q64_CS_HIGH();

    W25Q64_CS_LOW();
    SPI_Transfer(CMD_PAGE_PROG);
    SPI_Transfer((uint8_t)(addr >> 16));
    SPI_Transfer((uint8_t)(addr >> 8));
    SPI_Transfer((uint8_t)(addr));
    uint16_t i;
    for (i = 0; i < len && i < 256; i++) SPI_Transfer(data[i]);
    W25Q64_CS_HIGH();

    W25Q64_WaitBusy();
}

void W25Q64_Read(uint32_t addr, uint8_t *buf, uint16_t len) {
    if (!w25q64_ok) { return; }
    W25Q64_CS_LOW();
    SPI_Transfer(CMD_READ_DATA);
    SPI_Transfer((uint8_t)(addr >> 16));
    SPI_Transfer((uint8_t)(addr >> 8));
    SPI_Transfer((uint8_t)(addr));
    uint16_t i;
    for (i = 0; i < len; i++) buf[i] = SPI_Transfer(0xFF);
    W25Q64_CS_HIGH();
}
