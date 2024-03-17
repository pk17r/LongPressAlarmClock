#ifndef PIN_DEFS_H
#define PIN_DEFS_H

#include "configuration.h"

// define pins
#if defined(MCU_IS_ESP32_S2_MINI)

  // FOR ESP32 S2 MINI MODULE
  // single core

  #define MCU_IS_ESP32

  const int TFT_COPI = 35;
  const int TFT_CLK = 36;
  const int TFT_CS = 34;
  const int TFT_RST = 33;  // Or set to -1 and connect to Arduino RESET pin
  const int TFT_DC = 38;
  const int TFT_BL = 17;  //  controls TFT Display backlight as output of PWM pin

  #if defined(TOUCHSCREEN_IS_XPT2046)
    const int TS_CIPO = 37;    // don't connect CIPO (MISO) to TFT
    const int TS_CS_PIN = 21;
    const int TS_IRQ_PIN = 18;
  #endif

  // Sqw Alarm Interrupt Pin
  const int SQW_INT_PIN = 7;
  const int SDA_PIN = 8;
  const int SCL_PIN = 9;
  const int BUTTON_PIN = 6;
  const int INC_BUTTON_PIN = 11;
  const int DEC_BUTTON_PIN = 10;
  // #define BUTTON_PIN_BITMASK 0x800000000  // 2^35 in hex
  const int LED_PIN = 5;
  // const int LED_BUILTIN = 15;   // pre-defined
  const int BUZZER_PIN = 4;
  const int DEBUG_PIN = 21;    // manually pull down to enable debug mode, watchdog reboot will not be used in debug mode



#elif defined(MCU_IS_ESP32_WROOM_DA_MODULE)

  // FOR ESP32 WROOM DA MODULE
  // dual core

  #define MCU_IS_ESP32

  const int TFT_COPI = 23;
  const int TFT_CLK = 18;
  const int TFT_CS = 16;
  const int TFT_RST = 27;  // Or set to -1 and connect to Arduino RESET pin
  const int TFT_DC = 26;
  const int TFT_BL = 14;  //  controls TFT Display backlight as output of PWM pin

  #if defined(TOUCHSCREEN_IS_XPT2046)
    const int TS_CIPO = 19;    // don't connect CIPO (MISO) to TFT
    const int TS_CS_PIN = 33;
    const int TS_IRQ_PIN = 34;
  #endif

  // Sqw Alarm Interrupt Pin
  const int SQW_INT_PIN = 4;
  const int SDA_PIN = 21;
  const int SCL_PIN = 22;
  const int BUTTON_PIN = 35;
  #define BUTTON_PIN_BITMASK 0x800000000  // 2^35 in hex
  const int INC_BUTTON_PIN = 34;
  const int DEC_BUTTON_PIN = 33;
  const int LED_PIN = 32;
  const int LED_BUILTIN = 2;
  const int BUZZER_PIN = 13;
  const int DEBUG_PIN = 12;    // manually pull down to enable debug mode, watchdog reboot will not be used in debug mode


#elif defined(MCU_IS_RASPBERRY_PI_PICO_W)


  // DEFINE PINS USED ON RASPBERRY PI PICO W
  // dual core

  #define MCU_IS_RP2040


  const int TFT_COPI = 19;
  const int TFT_CLK = 18;
  const int TFT_CS = 17;
  const int TFT_RST = 20;  // Or set to -1 and connect to Arduino RESET pin
  const int TFT_DC = 21;
  const int TFT_BL = 22;  //  controls TFT Display backlight as output of PWM pin

  #if defined(TOUCHSCREEN_IS_XPT2046)
    const int TS_CIPO = 16;    // don't connect CIPO (MISO) to TFT
    const int TS_CS_PIN = 26;
    const int TS_IRQ_PIN = 27;
  #endif

  // Sqw Alarm Interrupt Pin
  const int SQW_INT_PIN = 3;
  const int SDA_PIN = 4;
  const int SCL_PIN = 5;
  const int BUTTON_PIN = 14;
  const int INC_BUTTON_PIN = 12;
  const int DEC_BUTTON_PIN = 13;
  const int LED_PIN = 15;
  // const int LED_BUILTIN = 25;    // pre-defined
  const int BUZZER_PIN = 11;
  const int DEBUG_PIN = 10;    // manually pull down to enable debug mode, watchdog reboot will not be used in debug mode


#endif


#endif  // PIN_DEFS_H