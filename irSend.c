#include "IRremote.h"

#ifdef SENDING_SUPPORTED // from IRremoteBoardDefs.h
//+=============================================================================
void sendRaw(const unsigned int buf[], unsigned int len, unsigned int hz) {
    // Set IR carrier frequency
    enableIROut(hz);

    for (unsigned int i = 0; i < len; i++) {
        if (i & 1) {
            space(buf[i]);
        } else {
            mark(buf[i]);
        }
    }

    space(0);  // Always end with the LED off
}

void sendRaw_P(const unsigned int buf[], unsigned int len, unsigned int hz) {
#if !defined(__AVR__)
    sendRaw(buf,len,hz); // Let the function work for non AVR platforms
#else
    // Set IR carrier frequency
    enableIROut(hz);

    for (unsigned int i = 0; i < len; i++) {
        uint16_t duration = pgm_read_word_near(buf + sizeof(uint16_t) * i);
        if (i & 1) {
            space(duration);
        } else {
            mark(duration);
        }
    }
    space(0);  // Always end with the LED off
#endif

}

#ifdef USE_SOFT_SEND_PWM
void inline sleepMicros(unsigned long us) {
#ifdef USE_SPIN_WAIT
    sleepUntilMicros(micros() + us);
#else
    if (us > 0U) { // Is this necessary? (Official docu https://www.arduino.cc/en/Reference/DelayMicroseconds does not tell.)
        delay_us((unsigned int) us);
    }
#endif
}

void inline sleepUntilMicros(unsigned long targetTime) {
#ifdef USE_SPIN_WAIT
    while (micros() < targetTime)
    ;
#else
    unsigned long now = micros();
    if (now < targetTime) {
        sleepMicros(targetTime - now);
    }
#endif
}
#endif // USE_SOFT_SEND_PWM

//+=============================================================================
// Sends PulseDistance data from MSB to LSB
//
void sendPulseDistanceWidthData(unsigned int aOneMarkMicros, unsigned int aOneSpaceMicros, unsigned int aZeroMarkMicros,
        unsigned int aZeroSpaceMicros, unsigned long aData, uint8_t aNumberOfBits, bool aMSBfirst) {

    if (aMSBfirst) {  // Send the MSB first.
        // send data from MSB to LSB until mask bit is shifted out
        for (unsigned long mask = 1UL << (aNumberOfBits - 1); mask; mask >>= 1) {
            if (aData & mask) {
                DBG_PRINT("1\r\n");
                mark(aOneMarkMicros);
                space(aOneSpaceMicros);
            } else {
                DBG_PRINT("0\r\n");
                mark(aZeroMarkMicros);
                space(aZeroSpaceMicros);
            }
        }
        DBG_PRINTLN("");
    }
#if defined(LSB_FIRST_REQUIRED)
    else {  // Send the Least Significant Bit (LSB) first / MSB last.
        for (uint16_t bit = 0; bit < aNumberOfBits; bit++, aData >>= 1)
            if (aData & 1) {  // Send a 1
                DBG_PRINT("1\r\n");
                mark(aOneMarkMicros);
                space(aOneSpaceMicros);
            } else {  // Send a 0
                DBG_PRINT("0\r\n");
                mark(aZeroMarkMicros);
                space(aZeroSpaceMicros);
            }
        DBG_PRINTLN("");
    }
#endif
}

//+=============================================================================
// Sends an IR mark for the specified number of microseconds.
// The mark output is modulated at the PWM frequency.
//

void mark(uint16_t timeMicros) {
#ifdef USE_SOFT_SEND_PWM
    unsigned long start = micros();
    unsigned long stop = start + timeMicros;
    if (stop + periodTimeMicros < start) {
        // Counter wrap-around, happens very seldom, but CAN happen.
        // Just give up instead of possibly damaging the hardware.
        return;
    }
    unsigned long nextPeriodEnding = start;
    unsigned long now = micros();
    while (now < stop) {
        SENDPIN_ON();
        sleepMicros (periodOnTimeMicros);
        SENDPIN_OFF();
        nextPeriodEnding += periodTimeMicros;
        sleepUntilMicros(nextPeriodEnding);
        now = micros();
    }
#elif defined(USE_NO_SEND_PWM)
    IR_SENDPIN_OFF(); // Set output to active low.
#else
    TIMER_ENABLE_SEND_PWM; // Enable pin 3 PWM output
#endif
    if (timeMicros > 0) {
        delay_us(timeMicros);
    }
}

void mark_long(uint32_t timeMicros) {
#if defined(USE_NO_SEND_PWM)
    IR_SENDPIN_OFF(); // Set output to active low.
#else
    TIMER_ENABLE_SEND_PWM; // Enable pin 3 PWM output
#endif
    if (timeMicros > 0) {
        delay_us(timeMicros);
    }
}

//+=============================================================================
// Leave pin off for time (given in microseconds)
// Sends an IR space for the specified number of microseconds.
// A space is no output, so the PWM output is disabled.
//
void space(uint16_t timeMicros) {
#if defined(USE_NO_SEND_PWM)
    IR_SENDPIN_ON; // Set output to inactive high.
#else
    TIMER_DISABLE_SEND_PWM; // Disable PWM output
#endif
    if (timeMicros > 0) {
        delay_us(timeMicros);
    }
}

/*
 * used e.g. by LEGO
 */
void space_long(uint32_t timeMicros) {
#if defined(USE_NO_SEND_PWM)
    IR_SENDPIN_ON; // Set output to inactive high.
#else
    TIMER_DISABLE_SEND_PWM; // Disable PWM output
#endif
    if (timeMicros > 0) {
        // custom delay does not work on an ATtiny85 with 1 MHz. It results in a delay of 760 us instead of the requested 560 us
        delay_us(timeMicros);
    }
}

#ifdef USE_DEFAULT_ENABLE_IR_OUT
//+=============================================================================
// Enables IR output.  The khz value controls the modulation frequency in kilohertz.
// The IR output will be on pin 3 (OC2B).
// This routine is designed for 36-40KHz; if you use it for other values, it's up to you
// to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
// TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
// controlling the duty cycle.
// There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
// To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
// A few hours staring at the ATmega documentation and this will all make sense.
// See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.
//
void enableIROut(int khz) {
#ifdef USE_SOFT_SEND_PWM
    periodTimeMicros = (1000U + khz / 2) / khz; // = 1000/khz + 1/2 = round(1000.0/khz)
    periodOnTimeMicros = periodTimeMicros * IR_SEND_DUTY_CYCLE / 100U - PULSE_CORRECTION_MICROS;
#endif

#if defined(USE_NO_SEND_PWM)
    IR_SENDPIN_ON; // Set output to inactive high.
#else
// Disable the Timer2 Interrupt (which is used for receiving IR)
    TIMER_DISABLE_RECEIVE_INTR; //Timer2 Overflow Interrupt

    IR_SENDPIN_OFF; // When not sending, we want it low

    timerConfigForSend(khz);
#endif
}
#endif

#endif // SENDING_SUPPORTED