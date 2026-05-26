#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "MPU6050.h"
#include "W25Q64.h"
#include "NTC.h"
#include "LightSensor.h"
#include "Filter.h"
#include "Alarm.h"
#include "Debug.h"

// Flash 配置结构，magic 校验数据有效性
typedef struct {
    float tempHigh;
    float tempLow;
    uint32_t magic;
} SysConfig;

static SysConfig cfg;
static float temperature;
static int16_t ax, ay, az;

static void Config_Load(void) {
    W25Q64_Read(0, (uint8_t *)&cfg, sizeof(cfg));
    // 首次上电 magic 不匹配，写默认值
    if (cfg.magic != 0xDEADBEEF) {
        cfg.tempHigh = 40.0f;
        cfg.tempLow  = -10.0f;
        cfg.magic    = 0xDEADBEEF;
        W25Q64_SectorErase(0);
        W25Q64_PageWrite(0, (uint8_t *)&cfg, sizeof(cfg));
    }
    g_Alarm.tempHigh = cfg.tempHigh;
    g_Alarm.tempLow  = cfg.tempLow;
}

static void Config_Save(void) {
    cfg.tempHigh = g_Alarm.tempHigh;
    cfg.tempLow  = g_Alarm.tempLow;
    W25Q64_SectorErase(0);
    W25Q64_PageWrite(0, (uint8_t *)&cfg, sizeof(cfg));
}

static void Btn_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef g = {GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12,
                          GPIO_Speed_50MHz, GPIO_Mode_IPU};
    GPIO_Init(GPIOB, &g);
}

// 按键扫描：按下接地，内部上拉。20ms消抖，等待释放后返回
static uint8_t Btn_Read(uint16_t pin) {
    if (GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET) {
        Delay_ms(20);  // 消抖
        if (GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET) {
            while (GPIO_ReadInputDataBit(GPIOB, pin) == Bit_RESET);  // 等释放
            return 1;
        }
    }
    return 0;
}

int main(void) {
    // 上电等待，让各模块供电稳定（约100ms@72MHz）
    for (volatile uint32_t i = 0; i < 720000; i++);

    Debug_Init(115200);
    Debug_Printf("\r\n=== Env Monitor V2.0 ===\r\n");

    // 关闭JTAG, 释放PB3/PB4用作普通GPIO
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "System Init...");

    NTC_Init();
    LightSensor_Init();
    MPU6050_Init();
    W25Q64_Init();
    Alarm_Init();
    Filter_Init();
    Btn_Init();

    Config_Load();

    OLED_Clear();
    OLED_ShowString(0, 0, "Env Monitor V2.0");
    Delay_ms(1000);
    OLED_Clear();

    uint8_t page = 0;
    uint32_t lastTick = 0, dbgTick = 0;

    while (1) {
        // ---- 传感器采集 ----
        temperature = NTC_ReadTemp();
        float tempFiltered = Filter_Add(temperature);
        uint8_t light = LightSensor_GetPercent();
        MPU6050_ReadAccel(&ax, &ay, &az);

        // 串口调试输出，每 10 次循环一次（~2s）
        if (dbgTick % 10 == 0) {
            Debug_Printf("[%ds] T=%.1fC L=%d%% ", dbgTick/5, tempFiltered, light);
            Debug_Printf("A(%d,%d,%d) ", ax, ay, az);
            if (g_Alarm.alarmFlags) Debug_Printf("ALARM=0x%02X", g_Alarm.alarmFlags);
            Debug_Printf("\r\n");
        }
        dbgTick++;
        Alarm_Check(tempFiltered, ax, ay, az);

        // ---- 按键处理 ----
        if (Btn_Read(GPIO_Pin_10)) {
            page = (page + 1) % 2;
            OLED_Clear();
        }
        if (Btn_Read(GPIO_Pin_11)) {
            if (page == 0) { g_Alarm.tempHigh += 0.5f; Config_Save(); }
        }
        if (Btn_Read(GPIO_Pin_12)) {
            if (page == 0) { g_Alarm.tempHigh -= 0.5f; Config_Save(); }
        }

        // ---- OLED 双页显示 ----
        if (page == 0) {
            OLED_ShowString(0, 0, "Temp:");
            OLED_ShowFloat(36, 0, tempFiltered, 3, 1);
            OLED_ShowString(60, 0, "C");

            OLED_ShowString(0, 1, "Light:");
            OLED_ShowNum(42, 1, light, 3);
            OLED_ShowString(66, 1, "%");

            OLED_ShowString(0, 3, "Hi:");
            OLED_ShowFloat(18, 3, g_Alarm.tempHigh, 3, 1);
            OLED_ShowString(42, 3, "C");

            OLED_ShowString(64, 3, "Lo:");
            OLED_ShowFloat(82, 3, g_Alarm.tempLow, 3, 1);
            OLED_ShowString(114, 3, "C");

            if (g_Alarm.alarmFlags & 0x01)
                OLED_ShowString(0, 6, "! Hi Temp");
            else if (g_Alarm.alarmFlags & 0x02)
                OLED_ShowString(0, 6, "! Lo Temp");
            else
                OLED_ShowString(0, 6, "Normal");
        } else if (page == 1) {
            OLED_ShowString(0, 0, "-- Accel --");

            OLED_ShowString(0, 2, "X:");
            OLED_ShowSignedNum(18, 2, ax, 6);
            OLED_ShowString(0, 3, "Y:");
            OLED_ShowSignedNum(18, 3, ay, 6);
            OLED_ShowString(0, 4, "Z:");
            OLED_ShowSignedNum(18, 4, az, 6);

            OLED_ShowString(0, 6, "Tilt:");
            if (g_Alarm.alarmFlags & 0x04)
                OLED_ShowString(36, 6, "ALARM");
            else
                OLED_ShowString(36, 6, "OK");
        }

        Delay_ms(200);
    }
}
