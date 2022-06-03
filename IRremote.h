/**
 * @file IRremote.h
 * @brief Public API to the library.
 */

//******************************************************************************
// IRremote
// Version 2.0.1 June, 2015
// Initially coded 2009 Ken Shirriff http://www.righto.com
// Edited by Mitra to add new controller SANYO
//
// Interrupt code based on NECIRrcv by Joe Knapp
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
// Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
//
// JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
// LG added by Darryl Smith (based on the JVC protocol)
// Whynter A/C ARC-110WD added by Francesco Meschia
// MagiQuest added by E. Stuart Hicks (based on code by mpflaga - https://github.com/mpflaga/Arduino-IRremote/)
//******************************************************************************
#ifndef IRremote_h
#define IRremote_h

//------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "private/IRremoteInt.h"
#include "printf.h"

/****************************************************
 *                     PROTOCOLS
 ****************************************************/
//------------------------------------------------------------------------------
// Supported IR protocols
// Each protocol you include costs memory and, during decode, costs time
// Disable (set to 0) all the protocols you do not need/want!
//
#define DECODE_BOSEWAVE      1
#define SEND_BOSEWAVE        0

#define DECODE_DENON         1
#define SEND_DENON           0

#define DECODE_DISH          0 // NOT WRITTEN
#define SEND_DISH            0

#define DECODE_JVC           1
#define SEND_JVC             0

#define DECODE_LEGO_PF       1
#define SEND_LEGO_PF         0

#define DECODE_LG            1
#define SEND_LG              0

#define DECODE_MAGIQUEST     1
#define SEND_MAGIQUEST       0

//#define USE_NEC_STANDARD // remove comment to have the standard NEC decoding (LSB first) available.
#if defined(USE_NEC_STANDARD)
#define DECODE_NEC_STANDARD  1
#define DECODE_NEC           0
#define LSB_FIRST_REQUIRED
#else
#define DECODE_NEC_STANDARD  0
#define DECODE_NEC           1
#endif
#define SEND_NEC             0
#define SEND_NEC_STANDARD    0

#define DECODE_PANASONIC     1
#define SEND_PANASONIC       0

#define DECODE_RC5           1
#define SEND_RC5             0

#define DECODE_RC6           1
#define SEND_RC6             0

#define DECODE_SAMSUNG       1
#define SEND_SAMSUNG         0

#define DECODE_SANYO         1
#define SEND_SANYO           0 // NOT WRITTEN

#define DECODE_SHARP         1
#define SEND_SHARP           0

#define DECODE_SHARP_ALT     1
#define SEND_SHARP_ALT       0
#if SEND_SHARP_ALT
#define LSB_FIRST_REQUIRED
#endif

#define DECODE_SONY          1
#define SEND_SONY            0

#define DECODE_WHYNTER       1
#define SEND_WHYNTER         0

#define DECODE_HASH          1 // special decoder for all protocols

/**
 * An enum consisting of all supported formats.
 * You do NOT need to remove entries from this list when disabling protocols!
 */
typedef enum {
    UNKNOWN = -1,
    UNUSED = 0,
    BOSEWAVE,
    DENON,
    DISH,
    JVC,
    LEGO_PF,
    LG,
    MAGIQUEST,
    NEC_STANDARD,
    NEC,
    PANASONIC,
    RC5,
    RC6,
    SAMSUNG,
    SANYO,
    SHARP,
    SHARP_ALT,
    SONY,
    WHYNTER,
} ir_decode_type_t;

/**
 * Comment this out for lots of lovely debug output.
 */
// #define DEBUG 1
//------------------------------------------------------------------------------
// Debug directives
//
#ifdef DEBUG
#  define DBG_PRINT(...)    printf(__VA_ARGS__)
#  define DBG_PRINTLN(...)  printf(__VA_ARGS__);printf("\r\n")
#else
/**
 * If DEBUG, print the arguments, otherwise do nothing.
 */
#  define DBG_PRINT(...)
/**
 * If DEBUG, print the arguments as a line, otherwise do nothing.
 */
#  define DBG_PRINTLN(...)
#endif

//------------------------------------------------------------------------------
// Helper macro for getting a macro definition as string
//
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

//------------------------------------------------------------------------------
// Mark & Space matching functions
//
int MATCH(unsigned int measured, unsigned int desired);
int MATCH_MARK(uint16_t measured_ticks, unsigned int desired_us);
int MATCH_SPACE(uint16_t measured_ticks, unsigned int desired_us);

/****************************************************
 *                     RECEIVING
 ****************************************************/
/**
 * Results returned from the decoder
 */
typedef struct {
    ir_decode_type_t decode_type;  ///< UNKNOWN, NEC, SONY, RC5, ...
    uint16_t address;           ///< Used by Panasonic & Sharp6 NEC_standard [16-bits]
    uint32_t value;             ///< Decoded value / command [max 32-bits]
    uint16_t bits;              ///< Number of bits in decoded value
    uint16_t magnitude;         ///< Used by MagiQuest [16-bits]
    bool isRepeat;              ///< True if repeat of value is detected

    // next 3 values are copies of irparams values - see IRremoteint.h
    uint16_t *rawbuf;           ///< Raw intervals in 50uS ticks
    uint16_t rawlen;            ///< Number of records in rawbuf
    bool overflow;              ///< true if IR raw code too long
} ir_decode_results;

/**
 * DEPRECATED
 * Decoded value for NEC and others when a repeat code is received
 * Use Flag ir_decode_results.isRepeat (see above) instead
 */
#define REPEAT 0xFFFFFFFF


/**
 * Attempt to decode the recently receive IR signal
 * @param results ir_decode_results instance returning the decode, if any.
 * @return success of operation.
 */
bool IR_decode(ir_decode_results *results);

/**
 * Enable IR reception.
 */
void IR_enableIRIn(void);

/**
 * Disable IR reception.
 */
void IR_disableIRIn(void);

/**
 * Enable/disable blinking of BLINKLED pin
 */
void IR_blink(bool blinkflag);

/**
 * Returns status of reception
 * @return true if no reception is on-going.
 */
bool IR_isIdle(void);

/**
 * Returns status of reception and copies IR-data to ir_decode_results buffer if true.
 * @return true if data is available.
 */
bool IR_available(ir_decode_results *results);

/**
 * Called to re-enable IR reception.
 */
void IR_resume(void);

const char* IR_getProtocolString(ir_decode_results *results);
void IR_printResultShort(ir_decode_results *results);
void IR_printIRResultRaw(ir_decode_results *results);
void IR_printIRResultRawFormatted(ir_decode_results *results);
void IR_printIRResultAsCArray(ir_decode_results *results);
void IR_printIRResultAsCVariables(ir_decode_results *results);

/**
 * Print the result (second argument) as Pronto Hex on the Stream supplied as argument.
 * @param stream The Stream on which to write, often Serial
 * @param results the ir_decode_results as delivered from irrecv.decode.
 * @param frequency Modulation frequency in Hz. Often 38000Hz.
 */
void IR_dumpPronto(ir_decode_results *results, unsigned int frequency);
void IR_printIRResultAsPronto(ir_decode_results *results, unsigned int frequency);

bool IR_decodePulseDistanceData(ir_decode_results *results, uint8_t aNumberOfBits, uint8_t aStartOffset, unsigned int aBitMarkMicros,
        unsigned int aOneSpaceMicros, unsigned int aZeroSpaceMicros, bool aMSBfirst);

//......................................................................
#if DECODE_RC5
/**
 * Try to decode the recently received IR signal as an RC5 signal-
 * @param results ir_decode_results instance returning the decode, if any.
 * @return Success of the operation.
 */
bool IR_decodeRC5(ir_decode_results *results);
#endif
#if DECODE_RC6
bool IR_decodeRC6(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_NEC
bool IR_decodeNEC(ir_decode_results *results);
#endif
#if DECODE_NEC_STANDARD
bool IR_decodeNECStandard(ir_decode_results *results);
#endif

//......................................................................
#if DECODE_SONY
bool IR_decodeSony(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_PANASONIC
bool IR_decodePanasonic(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_JVC
bool IR_decodeJVC(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_SAMSUNG
bool IR_decodeSAMSUNG(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_WHYNTER
bool IR_decodeWhynter(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_LG
bool IR_decodeLG(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_SANYO
bool IR_decodeSanyo(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_DISH
    bool  decodeDish (ir_decode_results *results) ; // NOT WRITTEN
#endif
//......................................................................
#if DECODE_SHARP
bool IR_decodeSharp(ir_decode_results *results);
#endif
#if DECODE_SHARP_ALT
bool IR_decodeSharpAlt(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_DENON
bool IR_decodeDenon(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_LEGO_PF
bool IR_decodeLegoPowerFunctions(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_BOSEWAVE
bool IR_decodeBoseWave(ir_decode_results *results);
#endif
//......................................................................
#if DECODE_MAGIQUEST
bool IR_decodeMagiQuest(ir_decode_results *results);
#endif

/****************************************************
 *                     SENDING
 ****************************************************/
/**
 * Define to use no carrier PWM, just simulate an active low receiver signal.
 */
//#define USE_NO_SEND_PWM
/**
 * Define to use carrier PWM generation in software, instead of hardware PWM.
 */
//#define USE_SOFT_SEND_PWM
/**
 * If USE_SOFT_SEND_PWM, this amount is subtracted from the on-time of the pulses.
 */
#ifndef PULSE_CORRECTION_MICROS
#define PULSE_CORRECTION_MICROS 3
#endif
/**
 * If USE_SOFT_SEND_PWM, use spin wait instead of delayMicros().
 */
//#define USE_SPIN_WAIT

void IR_enableIROut(int khz);
void IR_sendPulseDistanceWidthData(unsigned int aOneMarkMicros, unsigned int aOneSpaceMicros, unsigned int aZeroMarkMicros,
        unsigned int aZeroSpaceMicros, unsigned long aData, uint8_t aNumberOfBits, bool aMSBfirst);
void mark(uint16_t timeMicros);
void IR_mark_long(uint32_t timeMicros);
void space(uint16_t timeMicros);
void IR_space_long(uint32_t timeMicros);
void IR_sendRaw(const unsigned int buf[], unsigned int len, unsigned int hz);
void IR_sendRaw_P(const unsigned int buf[], unsigned int len, unsigned int hz);

//......................................................................
#if SEND_RC5
void IR_sendRC5(uint32_t data, uint8_t nbits);
void IR_sendRC5ext(uint8_t addr, uint8_t cmd, bool toggle);
#endif
#if SEND_RC6
void IR_sendRC6(uint32_t data, uint8_t nbits);
#endif
//......................................................................
#if SEND_NEC || SEND_NEC_STANDARD
void IR_sendNECRepeat();
#endif
#if SEND_NEC
void IR_sendNEC(uint32_t data, uint8_t nbits, bool repeat);
#endif
#if SEND_NEC_STANDARD
void IR_sendNECStandard(uint16_t aAddress, uint8_t aCommand, uint8_t aNumberOfRepeats);
#endif
//......................................................................
#if SEND_SONY
void IR_sendSony(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_PANASONIC
void IR_sendPanasonic(unsigned int address, unsigned long data);
#endif
//......................................................................
#if SEND_JVC
// JVC does NOT repeat by sending a separate code (like NEC does).
// The JVC protocol repeats by skipping the header.
// To send a JVC repeat signal, send the original code value
//   and set 'repeat' to true
void IR_sendJVC(unsigned long data, int nbits, bool repeat);
#endif
//......................................................................
#if SEND_SAMSUNG
void IR_sendSAMSUNG(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_WHYNTER
void IR_sendWhynter(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_LG
void IR_sendLG(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_SANYO
    void  sendSanyo      ( ) ; // NOT WRITTEN
#endif
//......................................................................
#if SEND_DISH
void IR_sendDISH(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_SHARP
void IR_sendSharpRaw(unsigned long data, int nbits);
void IR_sendSharp(unsigned int address, unsigned int command);
#endif
#if SEND_SHARP_ALT
void IR_sendSharpAltRaw(unsigned int data, int nbits);
void IR_sendSharpAlt(uint8_t address, uint8_t command);
#endif
//......................................................................
#if SEND_DENON
void IR_sendDenon(unsigned long data, int nbits);
#endif
//......................................................................
#if SEND_LEGO_PF
void IR_sendLegoPowerFunctions(uint16_t data, bool repeat);
#endif
//......................................................................
#if SEND_BOSEWAVE
void IR_sendBoseWave(unsigned char code);
#endif
//......................................................................
#if SEND_MAGIQUEST
void IR_sendMagiQuest(unsigned long wand_id, unsigned int magnitude);
#endif

/**
 * Parse the string given as Pronto Hex, and send it a number of times given
 * as the second argument. Thereby the division of the Pronto Hex into
 * an intro-sequence and a repeat sequence is taken into account:
 * First the intro sequence is sent, then the repeat sequence is sent times-1 times.
 * However, if the intro sequence is empty, the repeat sequence is sent times times.
 * <a href="http://www.harctoolbox.org/Glossary.html#ProntoSemantics">Reference</a>.
 *
 * Note: Using this function is very wasteful for the memory consumption on
 * a small board.
 * Normally it is a much better ide to use a tool like e.g. IrScrutinizer
 * to transform Pronto type signals offline
 * to a more memory efficient format.
 *
 * @param str C type string (null terminated) containing a Pronto Hex representation.
 * @param times Number of times to send the signal.
 */
void IR_sendProntoStr(const char* str, unsigned int times);

void IR_sendPronto(const uint16_t* data, unsigned int length, unsigned int times);

#if defined(USE_SOFT_SEND_PWM) || defined(USE_NO_SEND_PWM)

#  if defined(USE_SOFT_SEND_PWM)
unsigned int periodTimeMicros;
unsigned int periodOnTimeMicros;

void sleepMicros(unsigned long us);
void sleepUntilMicros(unsigned long targetTime);
#  endif

#endif

#endif // IRremote_h
