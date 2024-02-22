#ifndef PIN_DEFS_H
#define PIN_DEFS_H


// SELECT DISPLAY
#define DISPLAY_IS_ST7789V
// #define DISPLAY_IS_ST7735
// #define DISPLAY_IS_ILI9341
// #define DISPLAY_IS_ILI9488

// SELECT IF TOUCHSCREEN IS PRESENT
#define TOUCHSCREEN_IS_XPT2046

// SELECT IF WIFI IS USED
#define WIFI_IS_USED



  // DEFINE PINS USED ON RASPBERRY PI PICO W


#define TFT_COPI 19
#define TFT_CLK 18
#define TFT_CS 17
#define TFT_RST 20  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 21
#define TFT_BL 22  //  controls TFT Display backlight as output of PWM pin

#if defined(TOUCHSCREEN_IS_XPT2046)
  #define TS_CIPO 16    // don't connect CIPO (MISO) to TFT
  #define TS_CS_PIN 26
  #define TS_IRQ_PIN 27
#endif

// Sqw Alarm Interrupt Pin
#define SQW_INT_PIN 3
#define SDA_PIN 4
#define SCL_PIN 5
#define BUTTON_PIN 14
#define LED_PIN 15
#define BUZZER_PIN 11


#endif  // PIN_DEFS_H