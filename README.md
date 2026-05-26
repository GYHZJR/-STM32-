# 基于 STM32 的多参数环境监测与报警终端

基于 STM32F103C8T6 的多传感器环境监测系统，支持温度、光照、倾斜三参数实时监测与分级报警。

## 硬件清单

| 器件 | 型号 | 用途 |
|------|------|------|
| 主控 | STM32F103C8T6 | 核心控制 |
| 显示 | 0.96寸 OLED (SSD1306 I2C) | 数据展示 |
| 姿态 | MPU6050 (GY-521) | 加速度/倾斜检测 |
| 存储 | W25Q64 (8MB SPI Flash) | 配置掉电保存 |
| 温度 | NTC 10K B=3950 | 环境温度 (ADC) |
| 光照 | 光敏电阻模块 | 亮度检测 (ADC) |
| 报警 | 蜂鸣器模块 + LED×3 | 分级声光报警 |

## 接线

```
I2C1 (OLED + MPU6050 共享):
  PB6(SCL) → OLED SCL + MPU6050 SCL  (4.7K 上拉至 3.3V)
  PB7(SDA) → OLED SDA + MPU6050 SDA  (4.7K 上拉至 3.3V)

SPI1 (W25Q64):
  PA4(CS) → W25Q64 CS
  PA5(SCK) → W25Q64 CLK
  PA6(MISO) → W25Q64 DO
  PA7(MOSI) → W25Q64 DI

ADC (NTC + 光敏):
  PA0 → NTC 模块 AO
  PA1 → 光敏模块 AO

GPIO 输出:
  PB0 → 蜂鸣器 I/O (3脚模块, 低电平触发)
  PB1 → 红色 LED (高温报警)
  PB4 → 黄色 LED (低温报警)
  PB3 → 蓝色 LED (倾斜报警)

GPIO 输入 (内部上拉, 按下接 GND):
  PB10 → 页面切换
  PB11 → 阈值+
  PB12 → 阈值-

USART1 (调试输出):
  PA9(TX) → USB转TTL RX  (115200bps)
```

## 核心特性

- **温度采集**：NTC + 12位 ADC + B-parameter 公式 + 3样本滑动平均滤波
- **光照监测**：光敏电阻分压 + ADC → 0~100%
- **倾斜检测**：MPU6050 加速度计，轴偏差法，~9° 触发
- **三级报警**：高温→红灯、低温→黄灯、倾斜→蓝灯 + 蜂鸣器
- **配置存储**：W25Q64 Flash，magic number 校验，掉电不丢
- **I2C 总线共享**：OLED(0x78) + MPU6050(0xD0) 共用 PB6/PB7，超时保护 + 总线恢复

## 开发环境

- MDK-ARM (Keil μVision 5)
- STM32F10x 标准外设库 V3.5.0
- 编译器：ARMCC V5

## 目录结构

```
├── Hardware/     # 硬件驱动层
│   ├── OLED.c/h        OLED 显示 + I2C 总线恢复
│   ├── MPU6050.c/h     MPU6050 加速度采集
│   ├── W25Q64.c/h      SPI Flash 读写
│   ├── NTC.c/h         NTC 温度采集
│   ├── LightSensor.c/h 光敏电阻
│   ├── Alarm.c/h       报警逻辑 (蜂鸣器+LED)
│   ├── Filter.c/h      滑动平均滤波
│   └── Debug.c/h       串口调试输出
├── User/
│   └── main.c         主循环 + 双页显示 + 按键
├── Libraries/          STM32F10x 标准外设库
├── System/             Delay
├── start/              CMSIS 启动文件
└── project.uvprojx     Keil 工程文件
```
