#include "lwipopts.h"
#include "uRTCLib.h"
#include "rtc.h"

// RTC constructor
RTC::RTC() {

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.setSDA(SDA_PIN);
  URTCLIB_WIRE.setSCL(SCL_PIN);
  URTCLIB_WIRE.begin();

  // set rtc model
  rtc_hw_.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231 HW
  Refresh();

  // setup DS3231 rtc for the first time, no problem doing it again
  FirstTimeRtcSetup();

  // Check if time is up to date
  PrintLn("Lost power status: ");
  if (rtc_hw_.lostPower()) {
    PrintLn("POWER FAILED. Clearing flag...");
    rtc_hw_.lostPowerClear();
  }
  else
    PrintLn("POWER OK");

  // Check whether OSC is set to use VBAT or not
  if (rtc_hw_.getEOSCFlag())
    PrintLn("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!");
  else
    PrintLn("Oscillator will use VBAT if VCC cuts off.");

  // // make RTC class object _second equal to rtcHw second; + 2 seconds to let time synchronization happen on first time 60 seconds hitting
  // second_ = rtc_hw_.second() + 2;

  // seconds interrupt pin
  pinMode(SQW_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INT_PIN), SecondsUpdateInterruptISR, RISING);

  // set sqw pin to trigger every second
  rtc_hw_.sqwgSetMode(URTCLIB_SQWG_1H);

  PrintLn("RTC Initialized!");
}

// setup DS3231 rtc for the first time, no problem doing it again
void RTC::FirstTimeRtcSetup() {
  // Set Oscillator to use VBAT when VCC turns off if not set
  if(rtc_hw_.getEOSCFlag()) {
    if(rtc_hw_.enableBattery())
      PrintLn("Enable Battery Success");
    else
      PrintLn("Enable Battery UNSUCCESSFUL!");
  }

  // disable 32K Pin Sq Wave out if on
  if(rtc_hw_.status32KOut()) {
    rtc_hw_.disable32KOut();
    PrintLn("disable32KOut() done");
  }

  // disable Sqw Pin Wave out if on
  if(rtc_hw_.sqwgMode() != URTCLIB_SQWG_OFF_1) {
    rtc_hw_.sqwgSetMode(URTCLIB_SQWG_OFF_1);
    PrintLn("stop sq wave on sqw pin. Mode set: URTCLIB_SQWG_OFF_1");
  }

  // clear alarms flags if any
  if(rtc_hw_.alarmTriggered(URTCLIB_ALARM_1)) {
    rtc_hw_.alarmClearFlag(URTCLIB_ALARM_1);
    PrintLn("URTCLIB_ALARM_1 alarm flag cleared.");
  }
  if(rtc_hw_.alarmTriggered(URTCLIB_ALARM_2)) {
    rtc_hw_.alarmClearFlag(URTCLIB_ALARM_2);
    PrintLn("URTCLIB_ALARM_2 alarm flag cleared.");
  }

  // we won't use RTC for alarm, disable if enabled
  if(rtc_hw_.alarmMode(URTCLIB_ALARM_1) != URTCLIB_ALARM_TYPE_1_NONE) {
    rtc_hw_.alarmDisable(URTCLIB_ALARM_1);
    PrintLn("URTCLIB_ALARM_1 disabled.");
  }
  if(rtc_hw_.alarmMode(URTCLIB_ALARM_2) != URTCLIB_ALARM_TYPE_2_NONE) {
    rtc_hw_.alarmDisable(URTCLIB_ALARM_2);
    PrintLn("URTCLIB_ALARM_2 disabled.");
  }

  // set rtcHw in 12 hour mode if not already
  if(rtc_hw_.hourModeAndAmPm() == 0)
    rtc_hw_.set_12hour_mode(true);
}

// protected function to refresh time from RTC HW and do basic power failure checks
void RTC::Refresh() {

  // refresh time in class object from RTC HW
  rtc_hw_.refresh();

  // make _second equal to rtcHw seconds -> should be 0
  second_ = rtc_hw_.second();

  // check for minute update
  if(rtc_hw_.minute() != minute_) {
    minute_ = rtc_hw_.minute();
    rtc_hw_min_update_ = true;
  }
  PrintLn("__RTC Refresh__ ");

  // Check whether RTC HW experienced a power loss and thereby know if time is up to date or not
  if (rtc_hw_.lostPower()) {
    PrintLn("RTC POWER FAILED. Time is not up to date!");
  }

  // Check whether RTC HW Oscillator is set to use VBAT or not
  if (rtc_hw_.getEOSCFlag()) {
    PrintLn("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!");
  }

}

// clock seconds interrupt ISR
void RTC::SecondsUpdateInterruptISR() {
  // update seconds
  second_++;
  // a flag for others that time has updated!
  rtc_hw_sec_update_ = true;

  #if !defined(MCU_IS_ESP32)
    // refresh time on rtc class object on new minute
    if(second_ >= 60)
      rtc->Refresh();
  #endif
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
  // pause interrupts
  // noInterrupts();
  // Set current time and date
  // RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
  rtc_hw_.set(second, minute, hour_24_hr_mode, dayOfWeek_Sun_is_1, day, month_Jan_is_1, year - 2000);
  PrintLn("Time set");
  // refresh time from RTC HW
  Refresh();
  delay(100);
  // set RTC HW back into 12 hour mode
  rtc_hw_.set_12hour_mode(true);
  // restart interrupts
  // interrupts();
}
