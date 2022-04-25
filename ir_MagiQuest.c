#include "IRremote.h"

// Based off the Magiquest fork of Arduino-IRremote by mpflaga
// https://github.com/mpflaga/Arduino-IRremote/

//==============================================================================
//
//
//                            M A G I Q U E S T
//
//
//==============================================================================

// MagiQuest packet is both Wand ID and magnitude of swish and flick
typedef union  {
    unsigned long long llword;
    struct {
        unsigned int magnitude;
        unsigned long wand_id;
        char padding;
        char scrap;	// just to pad the struct out to 64 bits so we can union with llword
    } cmd;
} magiquest_t;

#define MAGIQUEST_BITS        50     // The number of bits in the command itself
#define MAGIQUEST_PERIOD      1150   // Length of time a full MQ "bit" consumes (1100 - 1200 usec)
/*
 * 0 = 25% mark & 75% space across 1 period
 *     1150 * 0.25 = 288 usec mark
 *     1150 - 288 = 862 usec space
 * 1 = 50% mark & 50% space across 1 period
 *     1150 * 0.5 = 575 usec mark
 *     1150 - 575 = 575 usec space
 */
#define MAGIQUEST_ONE_MARK    575
#define MAGIQUEST_ONE_SPACE   575
#define MAGIQUEST_ZERO_MARK   288
#define MAGIQUEST_ZERO_SPACE  862

#define MAGIQUEST_MASK        (1ULL << (MAGIQUEST_BITS-1))

//+=============================================================================
//
#if SEND_MAGIQUEST
void sendMagiQuest(unsigned long wand_id, unsigned int magnitude) {
    magiquest_t data;

    data.llword = 0;
    data.cmd.wand_id = wand_id;
    data.cmd.magnitude = magnitude;

    // Set IR carrier frequency
    enableIROut(38);

    // Data
    for (unsigned long long mask = MAGIQUEST_MASK; mask > 0; mask >>= 1) {
            DBG_PRINT("1\r\n");
        if (data.llword & mask) {
            mark(MAGIQUEST_ONE_MARK);
            space(MAGIQUEST_ONE_SPACE);
        } else {
            DBG_PRINT("0\r\n");
            mark(MAGIQUEST_ZERO_MARK);
            space(MAGIQUEST_ZERO_SPACE);
        }
    }
    DBG_PRINT("\r\n");

    // Footer
    mark(MAGIQUEST_ZERO_MARK);
    space(0);  // Always end with the LED off
}
#endif

//+=============================================================================
//
#if DECODE_MAGIQUEST
bool decodeMagiQuest() {
    magiquest_t data;  // Somewhere to build our code
    unsigned int offset = 1;  // Skip the gap reading

    unsigned int mark_;
    unsigned int space_;
    unsigned int ratio_;

#if DEBUG
    char bitstring[MAGIQUEST_BITS*2];
    memset(bitstring, 0, sizeof(bitstring));
#endif

    // Check we have enough data
    if (results.rawlen < 2 * MAGIQUEST_BITS) {
        DBG_PRINT("Not enough bits to be a MagiQuest packet (%u < %u)\r\n", 
            irparams.rawlen, MAGIQUEST_BITS*2);
        return false;
    }

    // Read the bits in
    data.llword = 0;
    while (offset + 1 < results.rawlen) {
        mark_ = results.rawbuf[offset++];
        space_ = results.rawbuf[offset++];
        ratio_ = space_ / mark_;

        DBG_PRINT("mark=%u space=%u ratio=%u\r\n",
            mark_ * MICROS_PER_TICK, space_ * MICROS_PER_TICK, ratio_);

        if (MATCH_MARK(space_ + mark_, MAGIQUEST_PERIOD)) {
            if (ratio_ > 1) {
                // It's a 0
                data.llword <<= 1;
#if DEBUG
                bitstring[(offset/2)-1] = '0';
#endif
            } else {
                // It's a 1
                data.llword = (data.llword << 1) | 1;
#if DEBUG
                bitstring[(offset/2)-1] = '1';
#endif
            }
        } else {
            DBG_PRINT("MATCH_MARK failed\r\n");
            return false;
        }
    }
#if DEBUG
    DBG_PRINT("%u\r\n", bitstring);
#endif

    // Success
    results.decode_type = MAGIQUEST;
    results.bits = offset / 2;
    results.value = data.cmd.wand_id;
    results.magnitude = data.cmd.magnitude;

    DBG_PRINT("MQ: bits=%u value=%u magnitude=%u\r\n",
        results.bits, results.value, results.magnitude);

    return true;
}
#endif
