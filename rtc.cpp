#include "lwipopts.h"
#include "uRTCLib.h"
#include "rtc.h"

// RTC constructor
RTC::RTC() {

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.begin(SDA_PIN, SCL_PIN);
  
  // setup DS3231 rtc
  Ds3231RtcSetup();

  PrintLn("RTC Initialized!");
}

// setup DS3231 rtc
void RTC::Ds3231RtcSetup() {

  // set rtc model
  rtc_hw_.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231 HW
  Refresh();

  // Set Oscillator to use VBAT when VCC turns off if not set
  if(rtc_hw_.getEOSCFlag()) {

    #ifdef MORE_LOGS
    if(rtc_hw_.enableBattery())
      PrintLn("Enable Battery Success");
    else
      PrintLn("Enable Battery UNSUCCESSFUL!");
    #else
    rtc_hw_.enableBattery();
    #endif

    delay(100);
  }

  // disable 32K Pin Sq Wave out if on
  if(rtc_hw_.status32KOut()) {
    rtc_hw_.disable32KOut();
    #ifdef MORE_LOGS
    PrintLn("disable32KOut() done");
    #endif
    delay(100);
  }

  // set sqw pin to trigger every second
  rtc_hw_.sqwgSetMode(URTCLIB_SQWG_1H);
  delay(100);

  // clear alarms flags if any
  if(rtc_hw_.alarmTriggered(URTCLIB_ALARM_1)) {
    rtc_hw_.alarmClearFlag(URTCLIB_ALARM_1);
    #ifdef MORE_LOGS
    PrintLn("URTCLIB_ALARM_1 alarm flag cleared.");
    #endif
    delay(100);
  }
  if(rtc_hw_.alarmTriggered(URTCLIB_ALARM_2)) {
    rtc_hw_.alarmClearFlag(URTCLIB_ALARM_2);
    #ifdef MORE_LOGS
    PrintLn("URTCLIB_ALARM_2 alarm flag cleared.");
    #endif
    delay(100);
  }

  // we won't use RTC for alarm, disable if enabled
  if(rtc_hw_.alarmMode(URTCLIB_ALARM_1) != URTCLIB_ALARM_TYPE_1_NONE) {
    rtc_hw_.alarmDisable(URTCLIB_ALARM_1);
    #ifdef MORE_LOGS
    PrintLn("URTCLIB_ALARM_1 disabled.");
    #endif
    delay(100);
  }
  if(rtc_hw_.alarmMode(URTCLIB_ALARM_2) != URTCLIB_ALARM_TYPE_2_NONE) {
    rtc_hw_.alarmDisable(URTCLIB_ALARM_2);
    #ifdef MORE_LOGS
    PrintLn("URTCLIB_ALARM_2 disabled.");
    #endif
    delay(100);
  }

  // set rtcHw in 12 hour mode if not already
  if(rtc_hw_.hourModeAndAmPm() == 0) {
    rtc_hw_.set_12hour_mode(true);
    delay(100);
  }


  // Check if time is up to date
  PrintLn("Lost power status: ");
  if (rtc_hw_.lostPower()) {
    #ifdef MORE_LOGS
    PrintLn("POWER FAILED. Clearing flag...");
    #endif
    rtc_hw_.lostPowerClear();
    delay(100);
  }
  #ifdef MORE_LOGS
  else
    PrintLn("POWER OK");
  #endif

  #ifdef MORE_LOGS
  // Check whether OSC is set to use VBAT or not
  if (rtc_hw_.getEOSCFlag())
    PrintLn("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!");
  else
    PrintLn("Oscillator will use VBAT if VCC cuts off.");
  #endif

  // // make RTC class object _second equal to rtcHw second; + 2 seconds to let time synchronization happen on first time 60 seconds hitting
  // second_ = rtc_hw_.second() + 2;

  // seconds interrupt pin
  pinMode(SQW_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INT_PIN), SecondsUpdateInterruptISR, RISING);

}

// private function to refresh time from RTC HW and do basic power failure checks
void RTC::Refresh() {

  // refresh time in class object from RTC HW
  rtc_hw_.refresh();
  rtc_refresh_reqd_ = false;

  // make _second equal to rtcHw seconds -> should be 0
  second_ = rtc_hw_.second();

  // PrintLn("__RTC Refresh__ ");

  SetTodaysMinutes();

  #ifdef MORE_LOGS
  // Check whether RTC HW experienced a power loss and thereby know if time is up to date or not
  if (rtc_hw_.lostPower())
    PrintLn(__func__, "lostPower");
  // Check whether RTC HW Oscillator is set to use VBAT or not
  if(rtc_hw_.getEOSCFlag())
    PrintLn(__func__, "getEOSCFlag");   // Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!
  #endif

}

// clock seconds interrupt ISR
void IRAM_ATTR RTC::SecondsUpdateInterruptISR() {
  // update seconds
  second_++;
  // a flag for others that time has updated!
  rtc_hw_sec_update_ = true;

  // refresh time on rtc class object on new minute
  if(second_ >= 60) {
    rtc_hw_min_update_ = true;
    rtc_refresh_reqd_ = true;
  }
}

uint8_t RTC::minute() {
  if(rtc_refresh_reqd_)
    Refresh();
  return rtc_hw_.minute();
}

uint8_t RTC::hour() {
  if(rtc_refresh_reqd_)
    Refresh();
  return rtc_hw_.hour();
}

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
void RTC::SetRtcTimeAndDate(uint8_t second, uint8_t minute, uint8_t hour_24_hr_mode, uint8_t dayOfWeek_Sun_is_1, uint8_t day, uint8_t month_Jan_is_1, uint16_t year) {
  // set RTC HW into 24 hour mode
  #ifdef MORE_LOGS
  PrintLn("Time Update Values:");
  PrintLn("hour_24_hr_mode: ", hour_24_hr_mode);
  PrintLn("minute: ", minute);
  PrintLn("second: ", second);
  PrintLn("dayOfWeek_Sun_is_1: ", dayOfWeek_Sun_is_1);
  PrintLn("day: ", day);
  PrintLn("month_Jan_is_1: ", month_Jan_is_1);
  PrintLn("year: ", year);
  #endif
  // Set current time and date
  // RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
  rtc_hw_.set(second, minute, hour_24_hr_mode, dayOfWeek_Sun_is_1, day, month_Jan_is_1, year - 2000);
  // refresh time from RTC HW
  Refresh();
  // set RTC HW back into 12 hour mode
  rtc_hw_.set_12hour_mode(true);
  PrintLn(__func__);
  Refresh();
}

void RTC::SetTodaysMinutes() {
  uint16_t todays_minutes_temp = minute();
  if(hourModeAndAmPm() == 0) {
    // 24 hour mode
    todays_minutes_temp += hour() * 60;
  }
  else {
    // 12 hour mode
    if(hour() != 12)
      todays_minutes_temp += hour() * 60;
    if(hourModeAndAmPm() == 2)
      todays_minutes_temp += 12 * 60;
  }
  todays_minutes = todays_minutes_temp;
}

void RTC::DaysMinutesToClockTime(uint16_t todays_minutes_val, uint8_t &hour_mode_and_am_pm, uint8_t &hr, uint8_t &min) {
  if(todays_minutes_val >= 60 * 12) {
    hour_mode_and_am_pm = 2;
    todays_minutes_val -= 60 * 12;
  }
  else
    hour_mode_and_am_pm = 1;

  min = todays_minutes_val % 60;

  uint8_t hr_temp = todays_minutes_val / 60;

  if(hr_temp == 0)
    hr = 12;
  else
    hr = hr_temp;
}

uint16_t RTC::ClockTimeToDaysMinutes(uint8_t hour_mode_and_am_pm, uint8_t hr, uint8_t min) {
  uint16_t todays_minutes_temp = min;
  if(hour_mode_and_am_pm == 0) {
    // 24 hour mode
    todays_minutes_temp += hr * 60;
  }
  else {
    // 12 hour mode
    if(hr != 12)
      todays_minutes_temp += hr * 60;
    if(hour_mode_and_am_pm == 2)
      todays_minutes_temp += 12 * 60;
  }
  return todays_minutes_temp;
}
