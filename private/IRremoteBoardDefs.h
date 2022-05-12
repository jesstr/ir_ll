/**
 * @file IRremoteBoardDefs.h
 *
 * @brief All board specific information should be contained in this file.
 * It defines a number of macros, depending on the board, as determined by
 * pre-proccesor symbols.
 * It was previously contained within IRremoteInt.h.
 */
// IRremote
// Version 2.0.1 June, 2015
// Copyright 2009 Ken Shirriff
// For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
// Modified by Paul Stoffregen <paul@pjrc.com> to support other boards and timers
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// Whynter A/C ARC-110WD added by Francesco Meschia
// Sparkfun Pro Micro support by Alastair McCormack
//******************************************************************************
#ifndef IRremoteBoardDefs_h
#define IRremoteBoardDefs_h

#include <stdint.h>
#include <stdbool.h>
#include "gpio.h"
#include "delay.h"
#include "cmsis_os.h"

// Define some defaults, that some boards may like to override
// (This is to avoid negative logic, ! DONT_... is just awkward.)

/**
 * Defined if the standard enableIRIn function should be used.
 * Undefine for boards supplying their own.
 */
#define USE_DEFAULT_ENABLE_IR_IN

/**
 * Define if the current board supports sending.
 * Currently not used.
 */
#define SENDING_SUPPORTED

/**
 * Defined if the standard enableIROut function should be used.
 * Undefine for boards supplying their own.
 */
#define USE_DEFAULT_ENABLE_IR_OUT

/**
 * Defined if Input Capture mode for receive timer should be used.
 * Undefine for default 50us periodic mode.
 */
#define USE_TIMER_IC_MODE

/**
 * Duty cycle in percent for sent signals.
 */
#if ! defined(IR_SEND_DUTY_CYCLE)
#define IR_SEND_DUTY_CYCLE 30 // 30 saves power and is compatible to the old existing code
#endif

//------------------------------------------------------------------------------
// This first #ifdef statement contains defines for blinking the LED,
// as well as all other board specific information, with the exception of
// timers and the sending pin.

#ifdef DOXYGEN
/**
 * If defined, denotes pin number of LED that should be blinked during IR reception.
 * Leave undefined to disable blinking.
 */
#define BLINKLED

/**
 * Board dependent macro to turn BLINKLED on.
 */
#define BLINKLED_ON()

/**
 * Board dependent macro to turn BLINKLED off.
 */
#define BLINKLED_OFF()

#else
#define BLINKLED

#ifndef BLINKLED_GPIO_Port
#define BLINKLED_GPIO_Port  GPIOC
#endif
#ifndef BLINKLED_Pin
#define BLINKLED_Pin        LL_GPIO_PIN_2
#endif

#define BLINKLED_ON()   (LL_GPIO_SetOutputPin(BLINKLED_GPIO_Port, BLINKLED_Pin))
#define BLINKLED_OFF()  (LL_GPIO_ResetOutputPin(BLINKLED_GPIO_Port, BLINKLED_Pin))
#endif

//------------------------------------------------------------------------------
// microseconds per clock interrupt tick
#if ! defined(MICROS_PER_TICK)
#define MICROS_PER_TICK    50
#endif

//---------------------------------------------------------

#define millis(x)       osKernelSysTick(x)
#define delay_ms(x)     osDelay(x)
#define delay_us(x)     delay_us(x)

//---------------------------------------------------------

#ifndef IRSEND_GPIO_Port
#define IRSEND_GPIO_Port    GPIOA
#endif
#ifndef IRSEND_Pin
#define IRSEND_Pin          LL_GPIO_PIN_2
#endif
#ifndef IRSEND_GPIO_AF
#define IRSEND_GPIO_AF      LL_GPIO_AF_2
#endif
#define IR_SENDPIN_ON       (LL_GPIO_SetOutputPin(IRSEND_GPIO_Port, IRSEND_Pin))
#define IR_SENDPIN_OFF      (LL_GPIO_ResetOutputPin(IRSEND_GPIO_Port, IRSEND_Pin))

#ifndef IRRECEIVE_GPIO_Port
#define IRRECEIVE_GPIO_Port GPIOA
#endif
#ifndef IRRECEIVE_Pin
#define IRRECEIVE_Pin       LL_GPIO_PIN_1
#endif
#ifndef IRRECEIVE_GPIO_AF
#define IRRECEIVE_GPIO_AF   LL_GPIO_AF_2
#endif
#define IR_READPIN          (LL_GPIO_IsInputPinSet(IRRECEIVE_GPIO_Port, IRRECEIVE_Pin))

//---------------------------------------------------------

#define IR_SEND_TIM                 TIM2
#define IR_SEND_TIM_IRQn            TIM2_IRQn

#define IR_RECEIVE_TIM              TIM2
#define IR_RECEIVE_TIM_IRQn         TIM2_IRQn

#define TIMER_ENABLE_SEND_PWM       LL_TIM_CC_EnableChannel(IR_SEND_TIM, LL_TIM_CHANNEL_CH1)
#define TIMER_DISABLE_SEND_PWM      LL_TIM_CC_DisableChannel(IR_SEND_TIM, LL_TIM_CHANNEL_CH1)

#define TIMER_ENABLE_RECEIVE_INTR   NVIC_EnableIRQ(IR_RECEIVE_TIM_IRQn)
#define TIMER_DISABLE_RECEIVE_INTR  NVIC_DisableIRQ(IR_RECEIVE_TIM_IRQn)

void timerConfigForReceive(void);
void timerConfigForSend(uint16_t aFrequencyKHz);
bool IR_TimerIRQHandler(void);

//---------------------------------------------------------


#endif // ! IRremoteBoardDefs_h
