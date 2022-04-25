//==============================================================================
//    L       EEEEEE   EEEE    OOOO
//    L       E       E       O    O
//    L       EEEE    E  EEE  O    O
//    L       E       E    E  O    O    LEGO Power Functions
//    LLLLLL  EEEEEE   EEEE    OOOO     Copyright (c) 2016, 2017 Philipp Henkel
//==============================================================================

//+=============================================================================
//

typedef struct {
    uint16_t data;
    bool repeatMessage;
    uint8_t messageBitIdx;
    uint8_t repeatCount;
    uint16_t messageLength;
} LegoPfBitStreamEncoder;

// HIGH data bit = IR mark + high pause
// LOW data bit = IR mark + low pause
static const uint16_t LOW_BIT_DURATION = 421;
static const uint16_t HIGH_BIT_DURATION = 711;
static const uint16_t START_BIT_DURATION = 1184;
static const uint16_t STOP_BIT_DURATION = 1184;
static const uint8_t IR_MARK_DURATION = 158;
static const uint16_t HIGH_PAUSE_DURATION = HIGH_BIT_DURATION - IR_MARK_DURATION;
static const uint16_t LOW_PAUSE_DURATION = LOW_BIT_DURATION - IR_MARK_DURATION;
static const uint16_t START_PAUSE_DURATION = START_BIT_DURATION - IR_MARK_DURATION;
static const uint16_t STOP_PAUSE_DURATION = STOP_BIT_DURATION - IR_MARK_DURATION;
static const uint8_t MESSAGE_BITS = 18;
static const uint16_t MAX_MESSAGE_LENGTH = 16000;

static uint16_t getMessageLength(LegoPfBitStreamEncoder *enc);
static uint16_t getDataBitPause(LegoPfBitStreamEncoder *enc);
static uint32_t getStopPause(LegoPfBitStreamEncoder *enc);
static uint32_t getStopPause(LegoPfBitStreamEncoder *enc);
static uint32_t getRepeatStopPause(LegoPfBitStreamEncoder *enc);


void reset(LegoPfBitStreamEncoder *enc, uint16_t data, bool repeatMessage) {
    enc->data = data;
    enc->repeatMessage = repeatMessage;
    enc->messageBitIdx = 0;
    enc->repeatCount = 0;
    enc->messageLength = getMessageLength(enc);
}

int getChannelId(LegoPfBitStreamEncoder *enc) {
    return 1 + ((enc->data >> 12) & 0x3);
}

uint16_t getMessageLength(LegoPfBitStreamEncoder *enc) {
    // Sum up all marks
    uint16_t length = MESSAGE_BITS * IR_MARK_DURATION;

    // Sum up all pauses
    length += START_PAUSE_DURATION;
    for (unsigned long mask = 1UL << 15; mask; mask >>= 1) {
        if (enc->data & mask) {
            length += HIGH_PAUSE_DURATION;
        } else {
            length += LOW_PAUSE_DURATION;
        }
    }
    length += STOP_PAUSE_DURATION;
    return length;
}

bool next(LegoPfBitStreamEncoder *enc) {
    enc->messageBitIdx++;
    if (enc->messageBitIdx >= MESSAGE_BITS) {
        enc->repeatCount++;
        enc->messageBitIdx = 0;
    }
    if (enc->repeatCount >= 1 && !enc->repeatMessage) {
        return false;
    } else if (enc->repeatCount >= 5) {
        return false;
    } else {
        return true;
    }
}

uint8_t getMarkDuration(void) {
    return IR_MARK_DURATION;
}

uint32_t getPauseDuration(LegoPfBitStreamEncoder *enc) {
    if (enc->messageBitIdx == 0)
        return START_PAUSE_DURATION;
    else if (enc->messageBitIdx < MESSAGE_BITS - 1) {
        return getDataBitPause(enc);
    } else {
        return getStopPause(enc);
    }
}

static uint16_t getDataBitPause(LegoPfBitStreamEncoder *enc) {
    const int pos = MESSAGE_BITS - 2 - enc->messageBitIdx;
    const bool isHigh = enc->data & (1 << pos);
    return isHigh ? HIGH_PAUSE_DURATION : LOW_PAUSE_DURATION;
}

static uint32_t getStopPause(LegoPfBitStreamEncoder *enc) {
    if (enc->repeatMessage) {
        return getRepeatStopPause(enc);
    } else {
        return STOP_PAUSE_DURATION;
    }
}

static uint32_t getRepeatStopPause(LegoPfBitStreamEncoder *enc) {
    if (enc->repeatCount == 0 || enc->repeatCount == 1) {
        return STOP_PAUSE_DURATION + (uint32_t) 5 * MAX_MESSAGE_LENGTH - enc->messageLength;
    } else if (enc->repeatCount == 2 || enc->repeatCount == 3) {
        return STOP_PAUSE_DURATION + (uint32_t) (6 + 2 * getChannelId(enc)) * MAX_MESSAGE_LENGTH - enc->messageLength;
    } else {
        return STOP_PAUSE_DURATION;
    }
}