#include "NTC.h"
#include <math.h>

/* NTC module: VCC(3.3V) → 10K fixed → ADC → NTC(10K@25°C, B=3950) → GND */
#define NTC_R_FIXED  10000.0f   // 10K fixed resistor
#define NTC_R0       10000.0f   // NTC resistance at T0
#define NTC_T0       298.15f    // 25°C in Kelvin
#define NTC_B        3950.0f    // B-constant

void NTC_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    // PA0: analog input
    GPIO_InitTypeDef g = {GPIO_Pin_0, GPIO_Speed_50MHz, GPIO_Mode_AIN};
    GPIO_Init(GPIOA, &g);

    // ADC1 config
    ADC_InitTypeDef adc;
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc);

    ADC_Cmd(ADC1, ENABLE);

    // Calibrate
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

// 电路：3.3V → 10K固定电阻 → ADC(PA0) → NTC → GND
// NTC温度↑ → 电阻↓ → ADC电压↓（负温度系数）
float NTC_ReadTemp(void) {
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    uint16_t adcVal = ADC_GetConversionValue(ADC1);
    float vout = adcVal * 3.3f / 4096.0f;

    if (vout < 0.01f) vout = 0.01f;    // 防除零
    if (vout > 3.29f) vout = 3.29f;

    // Vout = VCC * R_NTC / (R_fixed + R_NTC) → R_NTC = R_fixed * Vout / (VCC - Vout)
    float rNtc = NTC_R_FIXED * vout / (3.3f - vout);

    // B-parameter: 1/T = 1/T0 + ln(R/R0)/B
    float tempK = 1.0f / (1.0f / NTC_T0 + logf(rNtc / NTC_R0) / NTC_B);
    return tempK - 273.15f;
}
