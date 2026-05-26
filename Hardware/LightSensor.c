#include "LightSensor.h"

// 光敏电阻模块 PA1(ADC_CH1)，与 NTC 共用 ADC1（分时采集）
void LightSensor_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    GPIO_InitTypeDef g = {GPIO_Pin_1, GPIO_Speed_50MHz, GPIO_Mode_AIN};
    GPIO_Init(GPIOA, &g);
}

uint16_t LightSensor_Read(void) {
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

// 返回 0~100% 相对亮度（0=最暗，100=最亮）
uint8_t LightSensor_GetPercent(void) {
    uint16_t val = LightSensor_Read();
    if (val > 4095) val = 4095;
    return (uint8_t)(val * 100 / 4095);
}
