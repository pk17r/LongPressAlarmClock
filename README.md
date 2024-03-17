# Long Press Alarm Clock

![Alt text](resources/image_main.JPG?raw=true "Main Page")
![Alt text](resources/image_screensaver.JPG?raw=true "Screensaver Page")

Github: https://github.com/pk17r/Long_Press_Alarm_Clock/tree/release


- Hardware:
  - Microcontroller: ESP32 S2 Mini (Default) or ESP32 WROOM or Raspberry Pi Pico W
  - Display: 2.8" ST7789V display (Default), other selectable options: ST7735, ILI9341 and ILI9488
  - Touchscreen XPT2046 (not enabled by default)
  - DS3231 RTC Clock IC
  - AT24C32 EEPROM on DS3231 RTC Clock Module
  - A push button with LED
  - 2 push buttons for increase and decrease functions
  - An 85dB passive buzzer for alarm and different frequency tones


- Software:
  - All modules fully distributed in separate classes and files
  - Arduino setup and loop functions in .ino file
  - MCU Selection and Module selections in configuration.h file, pin definitions in pin_defs.h file
  - A fast very low RAM usage FastDrawBitmapSpi display function is implemented that converts a monochrome frame into RGB565 with 2 colors and sends a full frame to display via SPI row by row in only 50ms, achieving a FPS of 20 frames per second. This saves a lot of RAM on MCU. This way a 153kB RGB565 frame on a 320x240px display is reduced to just 9.6kB, allowing usage of lower RAM MCUs and faster processing times per frame.
  - Adafruit Library used for GFX functions
  - uRTCLib Library for DS3231 updated with AM/PM mode and class size reduced by 3 bytes while adding additional functionality
  - Secure Web OTA Firmware Update Functionality


- Salient Features
  - Long Press Alarm Clock: Program requires user to press and hold the main LED push button for 20 adjustable seconds continously to turn off alarm and buzzer, making sure user wakes up. There is no alarm snooze button.
  - C++ OOP Based Project
  - All modules have their own independent definition header files
  - A common header containing pointers to objects of every module and global functions
  - Time update via NTP server using WiFi once every day to maintain high accuracy
  - DS3231 RTC itself is high accuracy clock having deviation of +/-2 minutes per year
  - Time auto adjusts for time zone and day light savings with location ZIP/PIN and country code
  - Get Weather info using WiFi and display today's weather after alarm
  - Get user input of WiFi details via an on-screen keyboard (when touchscreen is used and enabled)
  - Colorful smooth Screensaver with a big clock
  - Touchscreen based alarm set page (touchscreen not on by default)
  - Settings saved in EEPROM so not lost on power loss
  - RP2040 watchdog keeps check on program not getting stuck, reboots if stuck
  - Screen brightness changes according to time of the day, with lowest brightness setting at night time
  - Modular programming that fits single core or dual core microcontrollers
  - Time critical tasks happen on core0 - time update, screensaver fast motion, alarm time trigger
  - Non Time critical tasks happen on core1 - update weather info using WiFi, update time using NTP server, connect/disconnect WiFi
  - Very Low Power usage of 0.4W during day and 0.3W during night time


- Datasheets:
  - ESP32 Lolin S2 Mini Single Core MCU Board https://www.wemos.cc/en/latest/s2/s2_mini.html
  - ESP32 Lolin S2 Mini Pinouts https://www.studiopieters.nl/wp-content/uploads/2022/08/WEMOS-ESP32-S2-MINI.png
  - 2.8" Touchscreen ST7789V driver https://www.aliexpress.us/item/3256805747165796.html
  - 2.8" Touchscreen ILI9341 driver http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807


  Prashant Kumar