#include "uRTCLib.h"
#include "rtc.h"

// RTC constructor
RTC::RTC() {

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.begin();

  // set rtc model
  rtcHw.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231 HW
  rtcHw.refresh();

  // setup DS3231 rtc for the first time, no problem doing it again
  firstTimeRtcSetup();

  // Check if time is up to date
  Serial.print(F("Lost power status: "));
  if (rtcHw.lostPower()) {
    Serial.println(F("POWER FAILED. Clearing flag..."));
    rtcHw.lostPowerClear();
  }
  else
    Serial.println(F("POWER OK"));

  // Check whether OSC is set to use VBAT or not
  if (rtcHw.getEOSCFlag())
    Serial.println(F("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!"));
  else
    Serial.println(F("Oscillator will use VBAT if VCC cuts off."));

  // make RTC class object _second equal to rtcHw second; + 2 seconds to let time synchronization happen on first time 60 seconds hitting
  _second = rtcHw.second() + 2;

  // seconds interrupt pin
  pinMode(SQW_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INT_PIN), secondsUpdateInterruptISR, RISING);

  // set sqw pin to trigger every second
  rtcHw.sqwgSetMode(URTCLIB_SQWG_1H);
}

// setup DS3231 rtc for the first time, no problem doing it again
void RTC::firstTimeRtcSetup() {
  // Set Oscillator to use VBAT when VCC turns off if not set
  if(!rtcHw.getEOSCFlag()) {
    if(rtcHw.enableBattery())
      Serial.println(F("Enable Battery Success"));
    else
      Serial.println(F("Enable Battery UNSUCCESSFUL!"));
  }

  // disable 32K Pin Sq Wave out if on
  if(rtcHw.status32KOut()) {
    rtcHw.disable32KOut();
    Serial.println(F("disable32KOut() done"));
  }

  // disable Sqw Pin Wave out if on
  if(rtcHw.sqwgMode() != URTCLIB_SQWG_OFF_1) {
    rtcHw.sqwgSetMode(URTCLIB_SQWG_OFF_1);
    Serial.println(F("stop sq wave on sqw pin. Mode set: URTCLIB_SQWG_OFF_1"));
  }

  // clear alarms flags if any
  if(rtcHw.alarmTriggered(URTCLIB_ALARM_1)) {
    rtcHw.alarmClearFlag(URTCLIB_ALARM_1);
    Serial.println(F("URTCLIB_ALARM_1 alarm flag cleared."));
  }
  if(rtcHw.alarmTriggered(URTCLIB_ALARM_2)) {
    rtcHw.alarmClearFlag(URTCLIB_ALARM_2);
    Serial.println(F("URTCLIB_ALARM_2 alarm flag cleared."));
  }

  // we won't use RTC for alarm, disable if enabled
  if(rtcHw.alarmMode(URTCLIB_ALARM_1) != URTCLIB_ALARM_TYPE_1_NONE) {
    rtcHw.alarmDisable(URTCLIB_ALARM_1);
    Serial.println(F("URTCLIB_ALARM_1 disabled."));
  }
  if(rtcHw.alarmMode(URTCLIB_ALARM_2) != URTCLIB_ALARM_TYPE_2_NONE) {
    rtcHw.alarmDisable(URTCLIB_ALARM_2);
    Serial.println(F("URTCLIB_ALARM_2 disabled."));
  }

  // set rtcHw in 12 hour mode if not already
  if(rtcHw.hourModeAndAmPm() == 0)
    rtcHw.set_12hour_mode(true);
}

void RTC::refresh() {

  // refresh time in class object from RTC HW
  rtcHw.refresh();
  Serial.println(F("__RTC Refresh__ "));

  // make _second equal to rtcHw seconds -> should be 0
  _second = rtcHw.second();

  // Check whether RTC HW experienced a power loss and thereby know if time is up to date or not
  if (rtcHw.lostPower()) {
    Serial.println(F("RTC POWER FAILED. Time is not up to date!"));
  }

  // Check whether RTC HW Oscillator is set to use VBAT or not
  if (rtcHw.getEOSCFlag()) {
    Serial.println(F("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!"));
  }

}

// clock seconds interrupt ISR
void RTC::secondsUpdateInterruptISR() {
  // update seconds
  _second++;
  // a flag for others that time has updated!
  rtcHwSecUpdate = true;
}

// to update rtcHw's time and date
void RTC::setRtcTimeAndDate() {
  // Set current time and date
  Serial.println();
  Serial.println(F("Waiting for input from user to set time."));
  Serial.println(F("Provide a keyboard input when set time is equal to real world time..."));
  while (Serial.available() == 0) {};
  // Only used once, then disabled
  rtcHw.set(0, 30, 2, 6, 26, 1, 24);
  //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
  Serial.println(F("Time set"));
  serialInputFlush();
}
