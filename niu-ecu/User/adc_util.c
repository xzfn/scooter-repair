

#include "adc_util.h"

#include "debug.h"


void adc_init(void)
{
    ADC_InitTypeDef  adc_struct = {0};
    GPIO_InitTypeDef gpio_struct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    // PC4/A2
    gpio_struct.GPIO_Pin = GPIO_Pin_4;
    gpio_struct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &gpio_struct);

    ADC_DeInit(ADC1);
    adc_struct.ADC_Mode = ADC_Mode_Independent;
    adc_struct.ADC_ScanConvMode = DISABLE;
    adc_struct.ADC_ContinuousConvMode = DISABLE;
    adc_struct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc_struct.ADC_DataAlign = ADC_DataAlign_Right;
    adc_struct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc_struct);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_241Cycles);
    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}


uint16_t adc_measure() {
    uint16_t adc_val = 0;
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {}  // wait
    adc_val = ADC_GetConversionValue(ADC1);
    return adc_val;
}
