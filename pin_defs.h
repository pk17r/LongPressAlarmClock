#ifndef PIN_DEFS_H
#define PIN_DEFS_H

// SELECT MCU
#define MCU_IS_ESP32
// #define MCU_IS_TEENSY

// SELECT DISPLAY
#define DISPLAY_IS_ST7789V
// #define DISPLAY_IS_ST7735
// #define DISPLAY_IS_ILI9341
// #define DISPLAY_IS_ILI9488

// SELECT IF TOUCHSCREEN IS PRESENT
#define TOUCHSCREEN_IS_XPT2046

// define pins
#if defined(MCU_IS_ESP32)

  #define TFT_COPI 23
  #define TFT_CLK 18
  #define TFT_CS 5
  #define TFT_RST 27  // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC 26
  #define TFT_BL 25  //  controls TFT Display backlight as output of PWM pin

  #if defined(TOUCHSCREEN_IS_XPT2046)
    #define TS_CIPO 19    // don't connect CIPO (MISO) to TFT
    #define TS_CS_PIN 33
    #define TS_IRQ_PIN 34
  #endif

  // Sqw Alarm Interrupt Pin
  #define SQW_INT_PIN 4
  #define BUTTON_PIN 35
  #define BUTTON_PIN_BITMASK 0x800000000  // 2^35 in hex
  #define LED_PIN 32

#elif defined(MCU_IS_TEENSY)

  #define TFT_COPI 11
  #define TFT_CLK 13
  #define TFT_CS 10
  #define TFT_RST 9  // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC 8
  #define TFT_BL 7  //  controls TFT Display backlight as output of PWM pin

  #if defined(TOUCHSCREEN_IS_XPT2046)
    #define TS_CIPO 12    // don't connect CIPO (MISO) to TFT
    #define TS_CS_PIN 5
    #define TS_IRQ_PIN 14
  #endif

  // Sqw Alarm Interrupt Pin
  #define SQW_INT_PIN 17
  #define BUTTON_PIN 2
  #define LED_PIN 14

#endif


#endif