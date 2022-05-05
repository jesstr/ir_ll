#include <stdint.h>
#include "IRremoteBoardDefs.h"
#include "stm32l0xx_ll_tim.h"
#include "stm32l0xx_ll_gpio.h"

#define TIM_SYSCLOCK	24000000 // Hz

#define TIM_PRESCALER	((TIM_SYSCLOCK) / 1000000 - 1) // 1Mhz

#ifdef USE_TIMER_IC_MODE
#define TIM_PERIOD		(10000 - 1) // 10ms
#else
#define TIM_PERIOD		(50 - 1) // 50us
#endif


void timerConfigForSend(uint16_t aFrequencyKHz)
{
	uint16_t pwm_freq = TIM_SYSCLOCK / (aFrequencyKHz * 1000) - 1;
	uint16_t pwm_pulse = pwm_freq / 3;

	TIM_TypeDef *TIMx = IR_SEND_TIM;

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

#ifdef USE_TIMER_IC_MODE
// Timer reconfiguration for Input Capture mode
static inline void timerConfigInputCaptureForReceive(void)
{
	TIM_TypeDef *TIMx = IR_RECEIVE_TIM;
	IRQn_Type IRQn = IR_RECEIVE_TIM_IRQn;

	NVIC_DisableIRQ(IRQn);
	LL_TIM_DeInit(TIMx);

	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Configure TIM2_CH2 GPIO pin
	GPIO_InitStruct.Pin = IRRECEIVE_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = IRRECEIVE_GPIO_AF;
	LL_GPIO_Init(IRRECEIVE_GPIO_Port, &GPIO_InitStruct);

	TIM_InitStruct.Prescaler = TIM_PRESCALER;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = TIM_PERIOD;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIMx, &TIM_InitStruct);

	LL_TIM_DisableARRPreload(TIMx);
	LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);
	LL_TIM_SetTriggerOutput(TIMx, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIMx);

	LL_TIM_IC_SetActiveInput(TIMx, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_INDIRECTTI);
	LL_TIM_IC_SetPrescaler(TIMx, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetFilter(TIMx, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
	LL_TIM_IC_SetPolarity(TIMx, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
	LL_TIM_IC_SetActiveInput(TIMx, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI);
	LL_TIM_IC_SetPrescaler(TIMx, LL_TIM_CHANNEL_CH2, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetFilter(TIMx, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV1);
	LL_TIM_IC_SetPolarity(TIMx, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_FALLING);

	NVIC_EnableIRQ(IRQn);
	LL_TIM_EnableIT_CC1(TIMx);
	LL_TIM_EnableIT_CC2(TIMx);

	LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_EnableChannel(TIMx, LL_TIM_CHANNEL_CH2);

	LL_TIM_EnableCounter(TIMx);
}
#else
// Timer reconfiguration for periodic mode
static inline void timerConfigPeriodicForReceive(void)
{
	TIM_TypeDef *TIMx = IR_RECEIVE_TIM;
	IRQn_Type IRQn = IR_RECEIVE_TIM_IRQn;

	NVIC_DisableIRQ(IRQn);
	LL_TIM_DeInit(TIMx);

	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Configure TIM2_CH1 GPIO pin
	GPIO_InitStruct.Pin = IRSEND_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = IRSEND_GPIO_AF;
	LL_GPIO_Init(IRSEND_GPIO_Port, &GPIO_InitStruct);

	TIM_InitStruct.Prescaler = TIM_PRESCALER;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = TIM_PERIOD;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIMx, &TIM_InitStruct);

	LL_TIM_DisableARRPreload(TIMx);
	LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);

	LL_TIM_SetTriggerOutput(TIMx, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIMx);

	NVIC_EnableIRQ(IRQn);
	LL_TIM_EnableIT_UPDATE(TIMx);

	LL_TIM_EnableCounter(TIMx);
}
#endif // USE_TIMER_IC_MODE

void timerConfigForReceive(void)
{
#ifdef USE_TIMER_IC_MODE
	timerConfigInputCaptureForReceive();
#else
	timerConfigPeriodicForReceive();
#endif // USE_TIMER_IC_MODE
}