#ifndef RTC_H
#define RTC_H

#include "common.h"
#include "uRTCLib.h"

class RTC {

public:

  RTC();  // constructor

  static inline volatile bool rtc_hw_sec_update_ = false;     // seconds flag triggered by interrupt
  static inline volatile bool rtc_hw_min_update_ = false;     // minutes change flag

  /**
  * \brief Sets RTC HW datetime data with input Hr in 24 hour mode and puts RTC to 12 hour mode
  *
  * @param second second
  * @param minute minute
  * @param hour_24_hr_mode hour to set in 24 hour mode
  * @param dayOfWeek_Sun_is_1 day of week with Sunday = 1
  * @param day today's date
  * @param month_Jan_is_1 month with January = 1
  * @param year year
  */
  void SetRtcTimeAndDate(uint8_t second, uint8_t minute, uint8_t hour_24_hr_mode, uint8_t dayOfWeek_Sun_is_1, uint8_t day, uint8_t month_Jan_is_1, uint16_t year);

  uint8_t second() { return second_; }
  uint8_t minute() { return rtc_hw_.minute(); }
  uint8_t hour() { return rtc_hw_.hour(); }
  uint8_t day() { return rtc_hw_.day(); }
  uint8_t month() { return rtc_hw_.month(); }
  uint16_t year() { return rtc_hw_.year() + 2000; }
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
  uint8_t dayOfWeek() { return rtc_hw_.dayOfWeek(); }
  /**
  * \brief Returns whether clock is in 12 or 24 hour mode
  * and AM or PM if in 12 hour mode
  * 0 = 24 hour mode (0-23 hours)
  * 1 = 12 hour mode AM hours (1-12 hours)
  * 2 = 12 hour mode PM hours (1-12 hours)
  *
  * @return byte with value 0, 1 or 2
  */
  uint8_t hourModeAndAmPm() { return rtc_hw_.hourModeAndAmPm(); }

#if !defined(MCU_IS_ESP32)
  protected:
#endif
  // protected function to refresh time from RTC HW and do basic power failure checks
  void Refresh();


private:

  // RTC clock object for DC3231 rtc
  uRTCLib rtc_hw_;

  // seconds counter to track RTC HW seconds, without
  // bothering it with I2C calls all the time.
  // we'll refresh RTC time everytime second reaches 60
  // All other parameters of RTC will not change at any other time
  // at 60 seconds, we'll update the time row
  static inline volatile uint8_t second_ = 0;

  // setup DS3231 rtc for the first time, no problem doing it again
  void FirstTimeRtcSetup();

  // clock seconds interrupt ISR
  static void SecondsUpdateInterruptISR();



};

#endif // RTC_H