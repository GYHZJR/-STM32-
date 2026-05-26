#include "MPU6050.h"
#include "OLED.h"

// ---- I2C 超时等待，避免总线卡死时死循环 ----
static int I2C_WaitFlag(uint32_t flag, FlagStatus target) {
    uint32_t t = 100000;
    while (I2C_GetFlagStatus(I2C1, flag) != target) {
        if (--t == 0) return -1;
    }
    return 0;
}

// I2C 错误恢复：发 STOP + 总线恢复
static void I2C_OnError(void) {
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_BusRecovery();
}

// 写 MPU6050 寄存器：START→地址(W)→寄存器→数据→STOP
static int I2C_WriteReg(uint8_t devAddr, uint8_t reg, uint8_t val) {
    if (I2C_WaitFlag(I2C_FLAG_BUSY, RESET)) { I2C_OnError(); return -1; }

    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitFlag(I2C_FLAG_SB, SET)) { I2C_OnError(); return -1; }
    I2C_Send7bitAddress(I2C1, devAddr, I2C_Direction_Transmitter);
    if (I2C_WaitFlag(I2C_FLAG_ADDR, SET)) { I2C_OnError(); return -1; }
    (void)I2C1->SR2;  // 读 SR2 清 ADDR 标志
    I2C_SendData(I2C1, reg);
    if (I2C_WaitFlag(I2C_FLAG_TXE, SET)) { I2C_OnError(); return -1; }
    I2C_SendData(I2C1, val);
    if (I2C_WaitFlag(I2C_FLAG_BTF, SET)) { I2C_OnError(); return -1; }
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}

// 读 MPU6050 多字节：START→reg地址→RESTART→读len字节→STOP
static int I2C_ReadMulti(uint8_t devAddr, uint8_t reg, uint8_t *buf, uint8_t len) {
    if (I2C_WaitFlag(I2C_FLAG_BUSY, RESET)) { I2C_OnError(); return -1; }

    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitFlag(I2C_FLAG_SB, SET)) { I2C_OnError(); return -1; }
    I2C_Send7bitAddress(I2C1, devAddr, I2C_Direction_Transmitter);
    if (I2C_WaitFlag(I2C_FLAG_ADDR, SET)) { I2C_OnError(); return -1; }
    (void)I2C1->SR2;
    I2C_SendData(I2C1, reg);
    if (I2C_WaitFlag(I2C_FLAG_TXE, SET)) { I2C_OnError(); return -1; }

    // 重复起始条件，切换为读方向
    I2C_GenerateSTART(I2C1, ENABLE);
    if (I2C_WaitFlag(I2C_FLAG_SB, SET)) { I2C_OnError(); return -1; }
    I2C_Send7bitAddress(I2C1, devAddr, I2C_Direction_Receiver);
    if (I2C_WaitFlag(I2C_FLAG_ADDR, SET)) { I2C_OnError(); return -1; }
    (void)I2C1->SR2;

    uint8_t i;
    for (i = 0; i < len; i++) {
        if (i == len - 1) I2C_AcknowledgeConfig(I2C1, DISABLE);  // 最后一字节 NACK
        if (I2C_WaitFlag(I2C_FLAG_RXNE, SET)) { I2C_OnError(); return -1; }
        buf[i] = I2C_ReceiveData(I2C1);
    }
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}

// 缓存上一次读数，通信失败时返回旧值，避免数据跳变
static int16_t lastAx, lastAy, lastAz;
static uint8_t mpuErrorCount = 0;

void MPU6050_Init(void) {
    mpuErrorCount = 0;
    // 0x6B=0x00: 退出睡眠，时钟选内部 8MHz
    if (I2C_WriteReg(MPU6050_ADDR, 0x6B, 0x00)) mpuErrorCount++;
    // 0x19=0x00: 采样率分频 = 8kHz/(1+0) = 8kHz
    if (I2C_WriteReg(MPU6050_ADDR, 0x19, 0x00)) mpuErrorCount++;
    // 0x1A=0x06: DLPF=6 → 加速度带宽 5Hz（去噪）
    if (I2C_WriteReg(MPU6050_ADDR, 0x1A, 0x06)) mpuErrorCount++;
    // 0x1C=0x00: 加速度量程 ±2g（16384 LSB/g，最灵敏）
    if (I2C_WriteReg(MPU6050_ADDR, 0x1C, 0x00)) mpuErrorCount++;
    // 0x1B=0x00: 陀螺仪量程 ±250°/s（未使用）
    if (I2C_WriteReg(MPU6050_ADDR, 0x1B, 0x00)) mpuErrorCount++;
    lastAx = lastAy = 0;
    lastAz = 16384;  // ±2g 时 1g 对应 16384
}

// 读加速度，失败时返回上次有效值
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    if (mpuErrorCount >= 5) {  // 连续 5 次失败，放弃读取
        *ax = lastAx; *ay = lastAy; *az = lastAz;
        return;
    }
    if (I2C_ReadMulti(MPU6050_ADDR, 0x3B, buf, 6) == 0) {
        lastAx = (int16_t)(buf[0] << 8 | buf[1]);
        lastAy = (int16_t)(buf[2] << 8 | buf[3]);
        lastAz = (int16_t)(buf[4] << 8 | buf[5]);
        mpuErrorCount = 0;
    } else {
        mpuErrorCount++;
    }
    *ax = lastAx;
    *ay = lastAy;
    *az = lastAz;
}
