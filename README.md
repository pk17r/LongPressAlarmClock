# Long Press Alarm Clock

![Alt text](resources/image_main.JPG?raw=true "Main Page")
![Alt text](resources/image_screensaver.JPG?raw=true "Screensaver Page")

- Hardware:
  - MCU: ESP32 S2 Mini (Default) or ESP32 WROOM or Raspberry Pi Pico W
  - Display: 2.8" ST7789V display (Default), other selectable options: ST7735, ILI9341 and ILI9488
  - Touchscreen XPT2046 (not enabled by default)
  - DS3231 RTC Clock
  - AT24C32 EEPROM on DS3231 RTC Clock Module
  - A push button with LED
  - 2 push buttons for increase and decrease functions
  - A quite powerful 85dB passive buzzer for alarm


- Software:
  - All modules fully distributed in separate classes and files
  - Arduino setup and loop functions in .ino file
  - MCU Selection and Module selections in configuration.h file, pin definitions in pin_defs.h file
  - A fast low RAM usage FastDrawBitmap function is implement that converts monochrome image into RGB565 with 2 colors and sends image to display via SPI row by row
  - Adafruit Library used for GFX functions
  - uRTCLib Library for DS3231 updated with AM/PM mode and class size reduced by 3 bytes while adding additional functionality
  - Web OTA Firmware Update Functionality


- Salient Features
  - Program requires user to press and hold a button for 25 seconds continously to turn off alarm and buzzer
  - C++ OOP Based Project
  - All modules have their own independent definition headers
  - A common header containing pointers to objects of every module and gloal functions
  - Time update via NTP server using WiFi once every day to keep accuracy
  - DS3231 RTC itself is high accuracy clock having deviation of +/-2 minutes per year
  - Get Weather info using WiFi and display today's weather after alarm
  - Get user input of WiFi details via an on-screen keyboard
  - Colorful smooth Screensaver with a big clock
  - Touchscreen based alarm set page (touchscreen not on by default)
  - Settings saved in EEPROM so not lost on power loss
  - RP2040 watchdog keeps check on program not getting stuck, reboots if stuck
  - Screen brightness changes according to time of the day, with lowest brightness setting at night time
  - Time critical tasks happen on core0 - time update, screensaver fast motion, alarm time trigger
  - Non Time critical tasks happen on core1 - update weather info using WiFi, update time using NTP server, connect/disconnect WiFi


- Datasheets:
  - ESP32 Lolin S2 Mini Single Core MCU Board https://www.wemos.cc/en/latest/s2/s2_mini.html
  - ESP32 Lolin S2 Mini Pinouts https://www.studiopieters.nl/wp-content/uploads/2022/08/WEMOS-ESP32-S2-MINI.png
  - 2.8" Touchscreen ST7789V driver https://www.aliexpress.us/item/3256805747165796.html
  - 2.8" Touchscreen ILI9341 driver http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807


  Prashant Kumar