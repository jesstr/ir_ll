//******************************************************************************
// IRremote
// Version 2.0.1 June, 2015
// Initially coded 2009 Ken Shirriff http://www.righto.com
//
// Modified by Paul Stoffregen <paul@pjrc.com> to support other boards and timers
// Modified  by Mitra Ardron <mitra@mitra.biz>
// Added Sanyo and Mitsubishi controllers
// Modified Sony to spot the repeat codes that some Sony's send
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// LG added by Darryl Smith (based on the JVC protocol)
// Whynter A/C ARC-110WD added by Francesco Meschia
//******************************************************************************

#include "IRremote.h"

struct irparams_struct irparams; // the irparams instance


//+=============================================================================
// The match functions were (apparently) originally MACROs to improve code speed
//   (although this would have bloated the code) hence the names being CAPS
// A later release implemented debug output and so they needed to be converted
//   to functions.
// I tried to implement a dual-compile mode (DEBUG/non-DEBUG) but for some
//   reason, no matter what I did I could not get them to function as macros again.
// I have found a *lot* of bugs in the Arduino compiler over the last few weeks,
//   and I am currently assuming that one of these bugs is my problem.
// I may revisit this code at a later date and look at the assembler produced
//   in a hope of finding out what is going on, but for now they will remain as
//   functions even in non-DEBUG mode
//
int MATCH(unsigned int measured, unsigned int desired) {
    bool passed = ((measured >= TICKS_LOW(desired)) && (measured <= TICKS_HIGH(desired)));
#if DEBUG
    DBG_PRINT("Testing: %u <= %u <= %u ? %s\r\n",
        TICKS_LOW(desired), measured, TICKS_HIGH(desired), passed ? "passed" : "FAILED");
#endif
    return passed;
}

//+========================================================
// Due to sensor lag, when received, Marks tend to be 100us too long
//
int MATCH_MARK(uint16_t measured_ticks, unsigned int desired_us) {
    // compensate for marks exceeded by demodulator hardware
    bool passed = ((measured_ticks >= TICKS_LOW(desired_us + MARK_EXCESS_MICROS))
            && (measured_ticks <= TICKS_HIGH(desired_us + MARK_EXCESS_MICROS)));
#if DEBUG
    DBG_PRINT("Testing mark (actual vs desired): %uus vs %uus: %u <= %u <= %u %s\r\n",
        measured_ticks * MICROS_PER_TICK, desired_us,
        TICKS_LOW(desired_us + MARK_EXCESS_MICROS) * MICROS_PER_TICK,
        measured_ticks * MICROS_PER_TICK,
        TICKS_HIGH(desired_us + MARK_EXCESS_MICROS) * MICROS_PER_TICK,
        passed ? "passed" : "FAILED");
#endif
    return passed;
}

//+========================================================
// Due to sensor lag, when received, Spaces tend to be 100us too short
//
int MATCH_SPACE(uint16_t measured_ticks, unsigned int desired_us) {
    // compensate for marks exceeded and spaces shortened by demodulator hardware
    bool passed = ((measured_ticks >= TICKS_LOW(desired_us - MARK_EXCESS_MICROS))
            && (measured_ticks <= TICKS_HIGH(desired_us - MARK_EXCESS_MICROS)));
#if DEBUG
    DBG_PRINT("Testing space (actual vs desired): %uus vs %uus: %u <= %u <= %u %s\r\n",
        measured_ticks * MICROS_PER_TICK, desired_us,
        TICKS_LOW(desired_us - MARK_EXCESS_MICROS) * MICROS_PER_TICK,
        measured_ticks * MICROS_PER_TICK,
        TICKS_HIGH(desired_us - MARK_EXCESS_MICROS) * MICROS_PER_TICK,
        passed ? "passed" : "FAILED");
#endif
    return passed;
}

//+=============================================================================
// Interrupt Service Routine - Fires every 50uS
// TIMER2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50uS [microseconds, 0.000050 seconds]
// 'rawlen' counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a the first [SPACE] entry gets long:
//   Ready is set; State switches to IDLE; Timing of SPACE continues.
// As soon as first MARK arrives:
//   Gap width is recorded; Ready is cleared; New logging starts
//
void IR_PeriodicTimerHandler(void) {
    TIMER_RESET_INTR_PENDING; // reset timer interrupt flag if required

    // Read if IR Receiver -> SPACE [xmt LED off] or a MARK [xmt LED on]
    uint8_t irdata = (uint8_t)IR_READPIN;

    irparams.timer++;  // One more 50uS tick
    if (irparams.rawlen >= RAW_BUFFER_LENGTH) {
        // Flag up a read overflow; Stop the State Machine
        irparams.overflow = true;
        irparams.rcvstate = IR_REC_STATE_STOP;
    }

    /*
     * Due to a ESP32 compiler bug https://github.com/espressif/esp-idf/issues/1552 no switch statements are possible for ESP32
     * So we change the code to if / else if
     */
//    switch (irparams.rcvstate) {
    //......................................................................
    if (irparams.rcvstate == IR_REC_STATE_IDLE) { // In the middle of a gap
        if (irdata == MARK) {\
            if (irparams.timer < GAP_TICKS) {  // Not big enough to be a gap.
                irparams.timer = 0;
            } else {
                // Gap just ended; Record gap duration; Start recording transmission
                // Initialize all state machine variables
                irparams.overflow = false;
                irparams.rawlen = 0;
                irparams.rawbuf[irparams.rawlen++] = irparams.timer;
                irparams.timer = 0;
                irparams.rcvstate = IR_REC_STATE_MARK;
            }
        }
    } else if (irparams.rcvstate == IR_REC_STATE_MARK) {  // Timing Mark
        if (irdata == SPACE) {   // Mark ended; Record time
            irparams.rawbuf[irparams.rawlen++] = irparams.timer;
            irparams.timer = 0;
            irparams.rcvstate = IR_REC_STATE_SPACE;
        }
    } else if (irparams.rcvstate == IR_REC_STATE_SPACE) {  // Timing Space
        if (irdata == MARK) {  // Space just ended; Record time
            irparams.rawbuf[irparams.rawlen++] = irparams.timer;
            irparams.timer = 0;
            irparams.rcvstate = IR_REC_STATE_MARK;

        } else if (irparams.timer > GAP_TICKS) {  // Space
            // A long Space, indicates gap between codes
            // Flag the current code as ready for processing
            // Switch to STOP
            // Don't reset timer; keep counting Space width
            irparams.rcvstate = IR_REC_STATE_STOP;
        }
    } else if (irparams.rcvstate == IR_REC_STATE_STOP) {  // Waiting; Measuring Gap
        if (irdata == MARK) {
            irparams.timer = 0;  // Reset gap timer
        }
    }

#ifdef BLINKLED
    // If requested, flash LED while receiving IR data
    if (irparams.blinkflag) {
        if (irdata == MARK) {
            BLINKLED_ON();   // if no user defined LED pin, turn default LED pin for the hardware on
        } else {
            BLINKLED_OFF();   // if no user defined LED pin, turn default LED pin for the hardware on
        }
    }
#endif // BLINKLED
}
