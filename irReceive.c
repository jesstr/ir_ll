#include "IRremote.h"

static bool IR_decodeHash(ir_decode_results *results);
static int compare(unsigned int oldval, unsigned int newval);


//+=============================================================================
// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
//
bool IR_decode(ir_decode_results *results) {
    if (irparams.rcvstate != IR_REC_STATE_STOP) {
        return false;
    }
    if (!results) {
        return false;
    }

    /*
     * First copy 3 values from irparams to internal results structure
     */
    results->rawbuf = irparams.rawbuf;
    results->rawlen = irparams.rawlen;
    results->overflow = irparams.overflow;

    // reset optional values
    results->address = 0;
    results->isRepeat = false;

#if DECODE_NEC_STANDARD
    DBG_PRINTLN("Attempting NEC_STANDARD decode");
    if (IR_decodeNECStandard(results)) {
        return true;
    }
#endif

#if DECODE_NEC
    DBG_PRINTLN("Attempting NEC decode");
    if (IR_decodeNEC(results)) {
        return true;
    }
#endif

#if DECODE_SHARP
    DBG_PRINTLN("Attempting Sharp decode");
    if (IR_decodeSharp(results)) {
        return true;
    }
#endif

#if DECODE_SHARP_ALT
    DBG_PRINTLN("Attempting SharpAlt decode");
    if (IR_decodeSharpAlt(results)) {
        return true;
    }
#endif

#if DECODE_SONY
    DBG_PRINTLN("Attempting Sony decode");
    if (IR_decodeSony(results)) {
        return true;
    }
#endif

#if DECODE_SANYO
    DBG_PRINTLN("Attempting Sanyo decode");
    if (IR_decodeSanyo(results)) {
        return true;
    }
#endif

#if DECODE_RC5
    DBG_PRINTLN("Attempting RC5 decode");
    if (IR_decodeRC5(results)) {
        return true;
    }
#endif

#if DECODE_RC6
    DBG_PRINTLN("Attempting RC6 decode");
    if (IR_decodeRC6(results)) {
        return true;
    }
#endif

#if DECODE_PANASONIC
    DBG_PRINTLN("Attempting Panasonic decode");
    if (IR_decodePanasonic(results)) {
        return true;
    }
#endif

#if DECODE_LG
    DBG_PRINTLN("Attempting LG decode");
    if (IR_decodeLG(results)) {
        return true;
    }
#endif

#if DECODE_JVC
    DBG_PRINTLN("Attempting JVC decode");
    if (IR_decodeJVC(results)) {
        return true;
    }
#endif

#if DECODE_SAMSUNG
    DBG_PRINTLN("Attempting SAMSUNG decode");
    if (IR_decodeSAMSUNG(results)) {
        return true;
    }
#endif

#if DECODE_WHYNTER
    DBG_PRINTLN("Attempting Whynter decode");
    if (IR_decodeWhynter(results)) {
        return true;
    }
#endif

#if DECODE_DENON
    DBG_PRINTLN("Attempting Denon decode");
    if (IR_decodeDenon(results)) {
        return true;
    }
#endif

#if DECODE_LEGO_PF
    DBG_PRINTLN("Attempting Lego Power Functions");
    if (IR_decodeLegoPowerFunctions(results)) {
        return true;
    }
#endif

#if DECODE_MAGIQUEST
    DBG_PRINTLN("Attempting MagiQuest decode");
    if (IR_decodeMagiQuest(results)) {
        return true;
    }
#endif

#if DECODE_HASH
    DBG_PRINTLN("Hash decode");
    // decodeHash returns a hash on any input.
    // Thus, it needs to be last in the list.
    // If you add any decodes, add them before this.
    if (IR_decodeHash(results)) {
        return true;
    }
#endif

    // Throw away and start over
    IR_resume();
    return false;
}

//+=============================================================================
// initialization
//
#ifdef USE_DEFAULT_ENABLE_IR_IN
void IR_enableIRIn(void) {
    // Setup timer mode and interrupts
    TIMER_DISABLE_RECEIVE_INTR;
    IR_timerConfigForReceive();
    TIMER_ENABLE_RECEIVE_INTR;

    // Initialize state machine state
    irparams.rcvstate = IR_REC_STATE_IDLE;
    irparams.rawlen = 0;
}

void IR_disableIRIn(void) {
    TIMER_DISABLE_RECEIVE_INTR;
}
#endif // USE_DEFAULT_ENABLE_IR_IN

//+=============================================================================
// Enable/disable blinking of BLINKLED pin 
//
void IR_blink(bool blinkflag) {
#ifdef BLINKLED
    irparams.blinkflag = blinkflag;
#endif
}

//+=============================================================================
// Return if receiving new IR signals
//
bool IR_isIdle(void) {
    return (irparams.rcvstate == IR_REC_STATE_IDLE || irparams.rcvstate == IR_REC_STATE_STOP) ? true : false;
}

bool IR_available(ir_decode_results *results) {
    if (irparams.rcvstate != IR_REC_STATE_STOP) {
        return false;
    }
    results->rawbuf = irparams.rawbuf;
    results->rawlen = irparams.rawlen;

    results->overflow = irparams.overflow;
    if (!results->overflow) {
        return true;
    }
    IR_resume(); //skip overflowed buffer
    return false;
}

//+=============================================================================
// Restart the ISR state machine
//
void IR_resume(void) {
    irparams.rcvstate = IR_REC_STATE_IDLE;
    irparams.rawlen = 0;
}

# if DECODE_HASH
//+=============================================================================
// hashdecode - decode an arbitrary IR code.
// Instead of decoding using a standard encoding scheme
// (e.g. Sony, NEC, RC5), the code is hashed to a 32-bit value.
//
// The algorithm: look at the sequence of MARK signals, and see if each one
// is shorter (0), the same length (1), or longer (2) than the previous.
// Do the same with the SPACE signals.  Hash the resulting sequence of 0's,
// 1's, and 2's to a 32-bit value.  This will give a unique value for each
// different code (probably), for most code systems.
//
// http://arcfn.com/2010/01/using-arbitrary-remotes-with-arduino.html
//
// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
// Use a tolerance of 20%
//
static int compare(unsigned int oldval, unsigned int newval) {
    if (newval * 10 < oldval * 8) {
        return 0;
    }
    if (oldval * 10 < newval * 8) {
        return 2;
    }
    return 1;
}

/*
 * Decode pulse distance protocols.
 * The mark (pulse) has constant length, the length of the space determines the bit value.
 * Each bit looks like: MARK + SPACE_1 -> 1
 *                 or : MARK + SPACE_0 -> 0
 * Data is read MSB first if not otherwise enabled.
 * Input is     results->rawbuf
 * Output is    results->value
 */
bool IR_decodePulseDistanceData(ir_decode_results *results, uint8_t aNumberOfBits, uint8_t aStartOffset, unsigned int aBitMarkMicros,
        unsigned int aOneSpaceMicros, unsigned int aZeroSpaceMicros, bool aMSBfirst) {
    unsigned long tDecodedData = 0;

    if (aMSBfirst) {
        for (uint8_t i = 0; i < aNumberOfBits; i++) {
            // Check for constant length mark
            if (!MATCH_MARK(results->rawbuf[aStartOffset], aBitMarkMicros)) {
                return false;
            }
            aStartOffset++;

            // Check for variable length space indicating a 0 or 1
            if (MATCH_SPACE(results->rawbuf[aStartOffset], aOneSpaceMicros)) {
                tDecodedData = (tDecodedData << 1) | 1;
            } else if (MATCH_SPACE(results->rawbuf[aStartOffset], aZeroSpaceMicros)) {
                tDecodedData = (tDecodedData << 1) | 0;
            } else {
                return false;
            }
            aStartOffset++;
        }
    }
#if defined(LSB_FIRST_REQUIRED)
    else {
        for (unsigned long mask = 1UL; aNumberOfBits > 0; mask <<= 1, aNumberOfBits--) {
            // Check for constant length mark
            if (!MATCH_MARK(results->rawbuf[aStartOffset], aBitMarkMicros)) {
                return false;
            }
            aStartOffset++;

            // Check for variable length space indicating a 0 or 1
            if (MATCH_SPACE(results->rawbuf[aStartOffset], aOneSpaceMicros)) {
                tDecodedData |= mask; // set the bit
            } else if (MATCH_SPACE(results->rawbuf[aStartOffset], aZeroSpaceMicros)) {
                // do not set the bit
            } else {
                return false;
            }

            aStartOffset++;
        }
    }
#endif
    results->value = tDecodedData;
    return true;
}

//+=============================================================================
// Use FNV hash algorithm: http://isthe.com/chongo/tech/comp/fnv/#FNV-param
// Converts the raw code values into a 32-bit hash code.
// Hopefully this code is unique for each button.
// This isn't a "real" decoding, just an arbitrary value.
//
#define FNV_PRIME_32 16777619
#define FNV_BASIS_32 2166136261

static bool IR_decodeHash(ir_decode_results *results) {
    long hash = FNV_BASIS_32;

    // Require at least 6 samples to prevent triggering on noise
    if (results->rawlen < 6) {
        return false;
    }

    for (unsigned int i = 1; (i + 2) < results->rawlen; i++) {
        int value = compare(results->rawbuf[i], results->rawbuf[i + 2]);
        // Add value into the hash
        hash = (hash * FNV_PRIME_32) ^ value;
    }

    results->value = hash;
    results->bits = 32;
    results->decode_type = UNKNOWN;

    return true;
}
#endif // defined(DECODE_HASH)

const char* IR_getProtocolString(ir_decode_results *results) {
    switch (results->decode_type) {
    default:
    case UNKNOWN:
        return ("UNKNOWN");
        break;
#if DECODE_BOSEWAVE
    case BOSEWAVE:
        return ("BOSEWAVE");
        break;
#endif
#if DECODE_DENON
    case DENON:
        return ("Denon");
        break;
#endif
#if DECODE_DISH
        case DISH:
        return("DISH");
        break;
#endif
#if DECODE_JVC
    case JVC:
        return ("JVC");
        break;
#endif
#if DECODE_LEGO_PF
    case LG:
        return("LEGO");
        break;
#endif
#if DECODE_LG
    case LEGO_PF:
        return ("LEGO_PF");
        break;
#endif
#if DECODE_MAGIQUEST
    case MAGIQUEST:
        return ("MAGIQUEST");
        break;
#endif
#if DECODE_NEC_STANDARD
    case NEC_STANDARD:
        return ("NEC_STANDARD");
        break;
#endif
#if DECODE_NEC
    case NEC:
        return("NEC");
        break;
#endif
#if DECODE_PANASONIC
    case PANASONIC:
        return ("PANASONIC");
        break;
#endif
#if DECODE_RC5
    case RC5:
        return ("RC5");
        break;
#endif
#if DECODE_RC6
    case RC6:
        return ("RC6");
        break;
#endif
#if DECODE_SAMSUNG
    case SAMSUNG:
        return ("SAMSUNG");
        break;
#endif
#if DECODE_SANYO
    case SANYO:
        return ("SANYO");
        break;
#endif
#if DECODE_SHARP
    case SHARP:
        return ("SHARP");
        break;
#endif
#if DECODE_SHARP_ALT
    case SHARP_ALT:
        return ("SHARP_ALT");
        break;
#endif
#if DECODE_SONY
    case SONY:
        return ("SONY");
        break;
#endif
#if DECODE_WHYNTER
    case WHYNTER:
        return ("WHYNTER");
        break;
#endif
    }
}

void IR_printResultShort(ir_decode_results *results) {
    DBG_PRINT("Protocol=%s Data=0x%X Address=0x%X %s\r\n",
        getProtocolString(results), results->value, results->address,
        results->isRepeat ? "R" : "");
}

void IR_printIRResultRaw(ir_decode_results *results) {
    // Dumps out the ir_decode_results structure.
    // Call this after decode()
    int count = results->rawlen;
    IR_printResultShort(results);

    DBG_PRINT("(%u bits) rawData[%u]:", results->bits, count);
    for (int i = 0; i < count; i++) {
        if (i & 1) {
            DBG_PRINT("%u", results->rawbuf[i] * MICROS_PER_TICK);
        } else {
            DBG_PRINT("-");
            DBG_PRINT("%u", (unsigned long)results->rawbuf[i] * MICROS_PER_TICK);
        }
        DBG_PRINT(" ");
    }
    DBG_PRINT("\r\n");
}

//+=============================================================================
// Dump out the ir_decode_results structure.
//
void IR_printIRResultRawFormatted(ir_decode_results *results) {
    // Print Raw data
    DBG_PRINT("rawData[%u]:\r\n", results->rawlen - 1);

    for (unsigned int i = 1; i < results->rawlen; i++) {
        unsigned long x = results->rawbuf[i] * MICROS_PER_TICK;
        if (!(i & 1)) {  // even
            DBG_PRINT("-");
            if (x < 1000) {
                DBG_PRINT(" ");
            }
            if (x < 100) {
                DBG_PRINT(" ");
            }
            DBG_PRINT("%u", x);
        } else {  // odd
            DBG_PRINT("     ");
            DBG_PRINT("+");
            if (x < 1000) {
                DBG_PRINT(" ");
            }
            if (x < 100) {
                DBG_PRINT(" ");
            }
            DBG_PRINT("%u", x);
            if (i < results->rawlen - 1) {
                DBG_PRINT(", "); //',' not needed for last one
            }
        }
        if (!(i % 8)) {
            DBG_PRINT("\r\n");
        }
    }
    DBG_PRINT("\r\n");
}

//+=============================================================================
// Dump out the ir_decode_results structure.
//
void IR_printIRResultAsCArray(ir_decode_results *results) {
    // Start declaration
    DBG_PRINT("uint16_t rawData[%u] = {", results->rawlen - 1);

    // Dump data
    for (unsigned int i = 1; i < results->rawlen; i++) {
        DBG_PRINT("%u", results->rawbuf[i] * MICROS_PER_TICK);
        if (i < results->rawlen - 1)
            DBG_PRINT(","); // ',' not needed on last one
        if (!(i & 1))
            DBG_PRINT(" ");
    }

    // End declaration
    DBG_PRINT("};");

    // Comment
    DBG_PRINT("  // ");
    IR_printResultShort(results);

    // Newline
    DBG_PRINT("\r\n");
}

void IR_printIRResultAsCVariables(ir_decode_results *results) {
    // Now dump "known" codes
    if (results->decode_type != UNKNOWN) {

        // Some protocols have an address
        if(results->address != 0){
            DBG_PRINT("uint16_t address = 0x%X;\r\n", results->address);
        }

        // All protocols have data
        DBG_PRINT("uint16_t data = 0x%x;\r\n", results->value);
        DBG_PRINT("\r\n");
    }
}