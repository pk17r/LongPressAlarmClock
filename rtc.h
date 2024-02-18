#ifndef RTC_H
#define RTC_H

#include "common.h"
#include "uRTCLib.h"

class RTC {

public:

  // seconds flag triggered by interrupt
  static inline volatile bool rtcHwSecUpdate = false;

  // constructor
  RTC();

  // to update rtc's time and date
  void setRtcTimeAndDate();

  void refresh();

  uint8_t second() { return _second; }
  uint8_t minute() { return rtcHw.minute(); }
  uint8_t hour() { return rtcHw.hour(); }
  uint8_t day() { return rtcHw.day(); }
  uint8_t month() { return rtcHw.month(); }
  uint16_t year() { return rtcHw.year() + 2000; }
  /**
  * \brief Returns actual Day Of Week
  *
  * @return Current stored Day Of Week
  *   - #URTCLIB_WEEKDAY_SUNDAY = 1
  *   - #URTCLIB_WEEKDAY_MONDAY = 2
  *   - #URTCLIB_WEEKDAY_TUESDAY = 3
  *   - #URTCLIB_WEEKDAY_WEDNESDAY = 4
  *   - #URTCLIB_WEEKDAY_THURSDAY = 5
  *   - #URTCLIB_WEEKDAY_FRIDAY = 6
  *   - #URTCLIB_WEEKDAY_SATURDAY = 7
  */
  uint8_t dayOfWeek() { return rtcHw.dayOfWeek(); }
  /**
  * \brief Returns whether clock is in 12 or 24 hour mode
  * and AM or PM if in 12 hour mode
  * 0 = 24 hour mode (0-23 hours)
  * 1 = 12 hour mode AM hours (1-12 hours)
  * 2 = 12 hour mode PM hours (1-12 hours)
  *
  * @return byte with value 0, 1 or 2
  */
  uint8_t hourModeAndAmPm() { return rtcHw.hourModeAndAmPm(); }


private:

  // RTC clock object for DC3231 rtc
  uRTCLib rtcHw;

  // seconds counter to track RTC HW seconds, without
  // bothering it with I2C calls all the time.
  // we'll refresh RTC time everytime second reaches 60
  // All other parameters of RTC will not change at any other time
  // at 60 seconds, we'll update the time row
  static inline uint8_t _second = 0;

  // setup DS3231 rtc for the first time, no problem doing it again
  void firstTimeRtcSetup();

  // clock seconds interrupt ISR
  static void secondsUpdateInterruptISR();



};

#endif // RTC_H