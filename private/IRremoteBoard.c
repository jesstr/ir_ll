#include <stdint.h>
#include "stm32l0xx_ll_tim.h"

// настройка таймера для приема - переполнение каждые 50 мкс (в данном случае системная частота 72Мгц)
#define MYPRESCALER 23 // получаем частоту 1МГц
#define MYPERIOD    49    // 50 мкс

// настройка таймера для отправки - указать системную частоту таймера
#define MYSYSCLOCK  24000000


void timerConfigForSend(uint16_t aFrequencyKHz)
{
	uint16_t pwm_freq = MYSYSCLOCK / (aFrequencyKHz * 1000) - 1;
	uint16_t pwm_pulse = pwm_freq / 3;

	TIM_TypeDef *TIMx = TIM2;

	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = pwm_freq;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIMx, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIMx);
	LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);
	LL_TIM_OC_EnablePreload(TIMx, LL_TIM_CHANNEL_CH1);

	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = pwm_pulse;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(TIMx, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIMx, LL_TIM_CHANNEL_CH1);
	LL_TIM_SetTriggerOutput(TIMx, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIMx);
}


void timerConfigForReceive(void) // initialization
{
	TIM_TypeDef *TIMx = TIM2;

	LL_TIM_DeInit(TIMx);

	LL_TIM_InitTypeDef TIM_InitStruct = {0};

	TIM_InitStruct.Prescaler = MYPRESCALER;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = MYPERIOD;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIMx, &TIM_InitStruct);

	LL_TIM_DisableARRPreload(TIMx);
	LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);

	LL_TIM_SetTriggerOutput(TIMx, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIMx);

	NVIC_EnableIRQ(TIM2_IRQn);
	LL_TIM_EnableIT_UPDATE(TIMx);

	LL_TIM_EnableCounter(TIMx);
}