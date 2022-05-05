# STM32 IR library (LL)

This library based on the IRremote Arduino library ([v2.8.0](https://github.com/z3t0/Arduino-IRremote/releases)) and enables you to send and receive infra-red signals.

No significant changes were made regarding to the original library code, except the following few things:
- rewritten in C language;
- added STM32L0 hardware initialization, based on LL drivers;
- removed support for all other architectures: AVR, ESP32, etc.
- added support for hardware input capture timer mode for reception;

Use input capture timer mode option (USE_TIMER_IC_MODE) if you don't wan't to use IRremote`s default periodical input pin polling technique.

Also refer to [the original homepage](http://z3t0.github.io/Arduino-IRremote/) for an additional info.

Library was designed for STM32L0 series, but can be easily adapted for any other MCU (see IRremoteBoard.c).