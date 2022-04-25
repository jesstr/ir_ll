/**
 * @file irPronto.cpp
 * @brief In this file, the functions dumpPronto and
 * sendPronto are defined.
 */

#include "IRremote.h"

// DO NOT EXPORT from this file
static const uint16_t MICROSECONDS_T_MAX = 0xFFFFU;
static const uint16_t learnedToken = 0x0000U;
static const uint16_t learnedNonModulatedToken = 0x0100U;
static const unsigned int bitsInHexadecimal = 4U;
static const unsigned int digitsInProntoNumber = 4U;
static const unsigned int numbersInPreamble = 4U;
static const unsigned int hexMask = 0xFU;
static const uint32_t referenceFrequency = 4145146UL;
static const uint16_t fallbackFrequency = 64767U; // To use with frequency = 0;
static const uint32_t microsecondsInSeconds = 1000000UL;
static const unsigned int RESULT_JUNK_COUNT = 1U;

static unsigned int toFrequencyKHz(uint16_t code) {
    return ((referenceFrequency / code) + 500) / 1000;
}

void sendPronto(const uint16_t *data, unsigned int size, unsigned int times) {
    unsigned int timebase = (microsecondsInSeconds * data[1] + referenceFrequency / 2) / referenceFrequency;
    unsigned int khz;
    switch (data[0]) {
    case learnedToken: // normal, "learned"
        khz = toFrequencyKHz(data[1]);
        break;
    case learnedNonModulatedToken: // non-demodulated, "learned"
        khz = 0U;
        break;
    default:
        return; // There are other types, but they are not handled yet.
    }
    unsigned int intros = 2 * data[2];
    unsigned int repeats = 2 * data[3];
    if (numbersInPreamble + intros + repeats != size) // inconsistent sizes
        return;

    unsigned int durations[intros + repeats];
    for (unsigned int i = 0; i < intros + repeats; i++) {
        uint32_t duration = ((uint32_t) data[i + numbersInPreamble]) * timebase;
        durations[i] = (unsigned int) ((duration <= MICROSECONDS_T_MAX) ? duration : MICROSECONDS_T_MAX);
    }

    unsigned int numberRepeats = intros > 0 ? times - 1 : times;
    if (intros > 0) {
        sendRaw(durations, intros - 1, khz);
    }

    if (numberRepeats == 0)
        return;

    delay_ms(durations[intros - 1] / 1000U);
    for (unsigned int i = 0; i < numberRepeats; i++) {
        sendRaw(durations + intros, repeats - 1, khz);
        if (i < numberRepeats - 1) { // skip last wait
            delay_ms(durations[intros + repeats - 1] / 1000U);
        }
    }
}

void sendProntoStr(const char *str, unsigned int times) {
    size_t len = strlen(str) / (digitsInProntoNumber + 1) + 1;
    uint16_t data[len];
    const char *p = str;
    char *endptr[1];
    for (unsigned int i = 0; i < len; i++) {
        long x = strtol(p, endptr, 16);
        if (x == 0 && i >= numbersInPreamble) {
            // Alignment error?, bail immediately (often right result).
            len = i;
            break;
        }
        data[i] = (uint16_t)(x); // If input is conforming, there can be no overflow!
        p = *endptr;
    }
    sendPronto(data, len, times);
}

static uint16_t effectiveFrequency(uint16_t frequency) {
    return frequency > 0 ? frequency : fallbackFrequency;
}

static uint16_t toTimebase(uint16_t frequency) {
    return microsecondsInSeconds / effectiveFrequency(frequency);
}

static uint16_t toFrequencyCode(uint16_t frequency) {
    return referenceFrequency / effectiveFrequency(frequency);
}

static void dumpDigit(unsigned int number) {
    DBG_PRINT("%X", number);
}

static void dumpNumber(uint16_t number) {
    for (unsigned int i = 0; i < digitsInProntoNumber; i++) {
        unsigned int shifts = bitsInHexadecimal * (digitsInProntoNumber - 1 - i);
        dumpDigit((number >> shifts) & hexMask);
    }
    DBG_PRINT(" ");
}

static void dumpDuration(uint16_t duration, uint16_t timebase) {
    dumpNumber((duration * MICROS_PER_TICK + timebase / 2) / timebase);
}

static void dumpSequence(const volatile uint16_t *data, size_t length, uint16_t timebase) {
    for (unsigned int i = 0; i < length; i++)
        dumpDuration(data[i], timebase);

    dumpDuration(_GAP, timebase);
}

/*
 * Using Print instead of Stream saves 1020 bytes program memory
 * Changed from & to * parameter type to be more transparent and consistent with other code of IRremote
 */
void dumpPronto(decode_results *results, unsigned int frequency) {
    dumpNumber(frequency > 0 ? learnedToken : learnedNonModulatedToken);
    dumpNumber(toFrequencyCode(frequency));
    dumpNumber((results->rawlen + 1) / 2);
    dumpNumber(0);
    unsigned int timebase = toTimebase(frequency);
    dumpSequence(results->rawbuf + RESULT_JUNK_COUNT, results->rawlen - RESULT_JUNK_COUNT, timebase);
}

//+=============================================================================
// Dump out the raw data as Pronto Hex.
// I know Stream * is locally inconsistent, but all global print functions use it
//
void printIRResultAsPronto(decode_results *results, unsigned int frequency) {
    DBG_PRINT("Pronto Hex: ");
    dumpPronto(results, frequency);
    DBG_PRINT("\r\n");
}
