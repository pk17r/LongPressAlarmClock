#ifndef CONFIGURATION_H
#define CONFIGURATION_H


// SELECT MCU

#define MCU_IS_ESP32_WROOM_DA_MODULE
// #define MCU_IS_ESP32_S2_MINI
// #define MCU_IS_RASPBERRY_PI_PICO_W


// SELECT DISPLAY

#define DISPLAY_IS_ST7789V
// #define DISPLAY_IS_ST7735
// #define DISPLAY_IS_ILI9341
// #define DISPLAY_IS_ILI9488


// SELECT IF TOUCHSCREEN IS PRESENT

// #define TOUCHSCREEN_IS_XPT2046


// SELECT IF WIFI IS USED

#define WIFI_IS_USED


// FIRMWARE VERSION   (update these when pushing new MCU specific binaries to github)

#define ESP32_WROOM_DA_MODULE_FIRMWARE_VERSION    "1.3"
#define ESP32_S2_MINI_FIRMWARE_VERSION            "1.3"
const std::string kFirmwareDate = "Mar 16, 2024";

#endif  // CONFIGURATION_H