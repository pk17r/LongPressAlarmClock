#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>

// SELECT MCU

#define MCU_IS_ESP32_S2_MINI
// #define MCU_IS_ESP32_WROOM_DA_MODULE
// #define MCU_IS_RASPBERRY_PI_PICO_W
// #define MCU_IS_ESP32_S3


// SELECT DISPLAY

#define DISPLAY_IS_ST7789V
// #define DISPLAY_IS_ST7735
// #define DISPLAY_IS_ILI9341
// #define DISPLAY_IS_ILI9488


// SELECT IF WIFI IS USED

#define WIFI_IS_USED


// FIRMWARE VERSION   (update these when pushing new MCU specific binaries to github)

#define ESP32_S2_MINI_FIRMWARE_VERSION            "2.5"
#define ESP32_WROOM_DA_MODULE_FIRMWARE_VERSION    "2.4"
#define RASPBERRY_PI_PICO_W_FIRMWARE_VERSION      "1.5"
#define ESP32_S3_FIRMWARE_VERSION                 "2.4"
const std::string kFirmwareDate = "Sep 15, 2024";

const std::string kChangeLog = "- RGB LEDs Settings\n- WiFi & Location Settings\n  Flow enhancements.";

#endif  // CONFIGURATION_H