#include "IRremote.h"
#include "ir_Lego_PF_BitStreamEncoder.h"

//==============================================================================
//    L       EEEEEE   EEEE    OOOO
//    L       E       E       O    O
//    L       EEEE    E  EEE  O    O
//    L       E       E    E  O    O    LEGO Power Functions
//    LLLLLL  EEEEEE   EEEE    OOOO     Copyright (c) 2016 Philipp Henkel
//==============================================================================

// Supported Devices
// LEGO® Power Functions IR Receiver 8884
/*
 * Lego Power Functions receive.
 * As per document
 * http://cache.lego.com/Media/Download/PowerfunctionsElementSpecsDownloads/otherfiles/download9FC026117C091015E81EC28101DACD4E/8884RemoteControlIRReceiver_Download.pdf

 * Receives the 16 bit protocol. It can be decoded with the  Open Powerfunctions code
 * https://bitbucket.org/tinkerer_/openpowerfunctionsrx
 */

//+=============================================================================
//
#if SEND_LEGO_PF
#if DEBUG
void logFunctionParameters(uint16_t data, bool repeat) {
  DBG_PRINT("sendLegoPowerFunctions(data=%u, repeat=%s\r\n",
    data, repeat ? "true" : "false");
}
#endif // DEBUG

void sendLegoPowerFunctions(uint16_t data, bool repeat) {
#if DEBUG
    logFunctionParameters(data, repeat);
#endif // DEBUG

    enableIROut(38);
    static LegoPfBitStreamEncoder enc;
    reset(&enc, data, repeat);
    do {
        mark(getMarkDuration());
        space_long(getPauseDuration(&enc));
    } while (next(&enc));
}

#endif // SEND_LEGO_PF

#if DECODE_LEGO_PF
/*
 * UNTESTED!!!
 */
#define LEGO_PF_STARTSTOP   1579
#define LEGO_PF_LOWBIT      526
#define LEGO_PF_HIBIT       947
#define LEGO_PF_LOWER       315
#define LEGO_PF_BITS        16  // The number of bits in the command

bool decodeLegoPowerFunctions(decode_results *results) {
    unsigned long data = 0;  // Somewhere to build our code
    DBG_PRINT("%u\r\n", results->rawlen);
    // Check we have the right amount of data
    if (irparams.rawlen != (2 * LEGO_PF_BITS) + 4)
        return false;

    DBG_PRINT("Attempting Lego Power Functions Decode\r\n");

    uint16_t desired_us = (results->rawbuf[1] + results->rawbuf[2]) * MICROS_PER_TICK;
    DBG_PRINT("PF desired_us = %u\r\n", desired_us);

    if (desired_us > LEGO_PF_HIBIT && desired_us <= LEGO_PF_STARTSTOP) {
        DBG_PRINT("Found PF Start Bit\r\n");
        int offset = 3;
        for (int i = 0; i < LEGO_PF_BITS; i++) {
            desired_us = (results->rawbuf[offset] + results->rawbuf[offset + 1]) * MICROS_PER_TICK;

            DBG_PRINT("PF desired_us = %u\r\n", desired_us);
            if (desired_us >= LEGO_PF_LOWER && desired_us <= LEGO_PF_LOWBIT) {
                DBG_PRINT("PF 0\r\n");
                data = (data << 1) | 0;
            } else if (desired_us > LEGO_PF_LOWBIT && desired_us <= LEGO_PF_HIBIT) {
                DBG_PRINT("PF 1\r\n");
                data = (data << 1) | 1;
            } else {
                DBG_PRINT("PF Failed\r\n");
                return false;
            }
            offset += 2;
        }

        desired_us = (results->rawbuf[offset]) * MICROS_PER_TICK;

        DBG_PRINT("PF END desired_us = %u\r\n", desired_us);
        if (desired_us < LEGO_PF_LOWER) {
            DBG_PRINT("Found PF End Bit %u\r\n", data);

            // Success
            results->bits = LEGO_PF_BITS;
            results->value = data;
            results->decode_type = LEGO_PF;
            return true;
        }
    }
    return false;
}
#endif
