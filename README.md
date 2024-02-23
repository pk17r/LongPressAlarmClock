# Touchscreen Long Press Alarm Clock


- Hardware:
  - MCU: Raspberry Pi Pico W
  - Display: 2.8" ST7789V touchscreen display, other selectable options: ST7735, ILI9341 and ILI9488
  - DS3231 RTC Clock
  - A push button with LED
  - A quite powerful 85dB passive buzzer for alarm


- Software:
  - A fast low RAM usage FastDrawBitmap function is implement that converts monochrome image into RGB565 with 2 colors and sends image to display via SPI row by row
  - Adafruit Library used for GFX functions
  - uRTCLib Library for DS3231 updated with AM/PM mode and class size reduced by 3 bytes while adding additional functionality


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
  - Touchscreen based alarm set page
  - Settings saved in EEPROM so not lost on power loss
  - RP2040 watchdog keeps check on program not getting stuck, reboots if stuck
  - Screen brightness changes according to time of the day, with lowest brightness setting at night time
  - Time critical tasks happen on core0 - time update, screensaver fast motion, alarm time trigger
  - Non Time critical tasks happen on core1 - update weather info using WiFi, update time using NTP server, connect/disconnect WiFi


  Prashant Kumar