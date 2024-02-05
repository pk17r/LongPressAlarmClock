/**************************************************************************
  Arduino Alarm Clock using 
    2.4" ILI9341 display and Adafruit Library and Functions
    DS3231 RTC Clock and uRTCLib Library and Functions
    RTC HW sends out a 1H wave so arduino can update time without talking to RTC HW
    TM1637 LED 4 digit clock display and TM1637Display Library and Functions

  Prashant Kumar

***************************************************************************/

#if defined(MCU_IS_ESP32)
#include "WiFi.h"
#endif
#include "pin_defs.h"
#include <PushButtonTaps.h>
#include "rgb_display_class.h"
#include "uRTCLib.h"
#include "alarm_clock_functions.cpp"

// Push Button
PushButtonTaps pushBtn;

// GLOBAL OBJECTS and VARIABLES

// RTC clock object for DC3231 rtc
uRTCLib rtc;

// display object
rgb_display_class display;

// seconds blinker
bool blink = false;

// seconds counter to track RTC HW seconds
// we'll refresh RTC time everytime second reaches 60
// All other parameters of RTC will not change at any other time
// at 60 seconds, we'll update the time row
byte second = 0;

// flag to refresh RTC time from RTC HW
bool refreshRtcTime = false;

// flag to set alarm On or Off
bool alarmOn = true;

// counter to note user inactivity seconds
uint8_t inactivitySeconds = 0;
const uint8_t INACTIVITY_SECONDS_LIM = 120;

// function declerations
void rtc_clock_initialize();
void serialTimeStampPrefix();
void updateSecond();
void serial_input_flush();
void processSerialInput();
// #if defined(MCU_IS_ESP32)
// void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason);
// void putEsp32ToLightSleep();
// #endif

// flag triggered by interrupt
volatile bool secondsIncremented = false;

// initialize static variable
// bool secondsIncremented = false;

// interrupt ISR
void sqwPinInterruptFn() {
  secondsIncremented = true;
}

// arduino setup function
void setup() {
#if defined(MCU_IS_ESP32)
  WiFi.mode(WIFI_OFF);
  delay(1);
  WiFi.disconnect();
  setCpuFrequencyMhz(80);
#endif
  Serial.begin(9600);
  delay(100);
  // while(!Serial) {};
  Serial.println(F("\nSerial OK"));

  // setup alarm clock program
  
  display.setup();

  rtc_clock_initialize();

  // seconds blink LED
  pinMode(LED_PIN, OUTPUT);
  
  // initialize push button
  pushBtn.setButtonPin(BUTTON_PIN);
  // pushBtn.setButtonActiveLow(false);

  // seconds interrupt pin
  pinMode(SQW_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INT_PIN), sqwPinInterruptFn, RISING);

  // prepare date and time arrays and serial print RTC Date Time
  display.prepareTimeDayDateArrays();

  // serial print RTC Date Time
  display.serialPrintRtcDateTime();

  // update TFT display
  display.displayUpdate();

  // set display brightness based on time of day
  display.checkTimeAndSetBrightness();

  // set sqw pin to trigger every second
  rtc.sqwgSetMode(URTCLIB_SQWG_1H);
}

// arduino loop function
void loop() {
  if(pushBtn.checkButtonStatus() != 0) {
    display.setBrightness(display.MAX_BRIGHTNESS);
    display.screensaverControl(false);
  }

  // process time actions every second
  if (secondsIncremented) {
    secondsIncremented = false;
    // update seconds
    ++second;
    if (second >= 60)
      refreshRtcTime = true;

    // prepare updated seconds array
    snprintf(display.newDisplayData.timeSS, display.SS_ARR_SIZE, ":%02d", second);

    serialTimeStampPrefix();

    // blink LED every second
    blink = !blink;
    digitalWrite(LED_PIN, blink);

    // get time update
    if (refreshRtcTime) {
      Serial.println(F("__RTC Refresh__ "));
      rtc.refresh();
      refreshRtcTime = false;

      // make second equal to rtc seconds -> should be 0
      second = rtc.second();

      serialTimeStampPrefix();

      // Check if time is up to date
      if (rtc.lostPower()) {
        Serial.println(F("POWER FAILED. Time is not up to date!"));
        Serial.println(F("Stopping!"));
        exit(1);
      }

      // Check whether OSC is set to use VBAT or not
      if (rtc.getEOSCFlag()) {
        Serial.println(F("Oscillator will not use VBAT when VCC cuts off. Time will not increment without VCC!"));
      }

      serialTimeStampPrefix();

      // if user has been inactive, then set brightness based on time of day
      // set display brightness based on time
      if(inactivitySeconds >= INACTIVITY_SECONDS_LIM)
        display.checkTimeAndSetBrightness();
    }

    // prepare date and time arrays
    display.prepareTimeDayDateArrays();

    // check for inactivity
    if(inactivitySeconds <= INACTIVITY_SECONDS_LIM) {
      inactivitySeconds++;
      if(inactivitySeconds >= INACTIVITY_SECONDS_LIM) {
        // set display brightness based on time
        display.checkTimeAndSetBrightness();
        // turn screen saver On
        if(!display.screensaver)
          display.screensaverControl(true);
      }
    }

    // update TFT display for changes
    if(!display.screensaver)
      display.displayUpdate();
    else
      display.displayHHMM();

    // serial print RTC Date Time
    display.serialPrintRtcDateTime();

    Serial.println();

#if defined(MCU_IS_ESP32)
    // if button is inactive, then go to sleep
    if(!pushBtn.buttonActiveDebounced())
      putEsp32ToLightSleep();
#endif
  }
  
  // accept user inputs
  if (Serial.available() != 0)
    processSerialInput();
}


#if defined(MCU_IS_ESP32)
/*
  Esp32 light sleep function
  https://lastminuteengineers.com/esp32-deep-sleep-wakeup-sources/
*/
void putEsp32ToLightSleep() {
  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */
  // add a timer to wake up ESP32
  esp_sleep_enable_timer_wakeup(500000); //0.5 seconds
  // ext1 button press as wake up source
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  //Go to sleep now
  serialTimeStampPrefix();
  Serial.println("Go To Light Sleep for 0.5 sec or button press");
  Serial.flush();
  // go to light sleep
  esp_light_sleep_start();
  // On WAKEUP disable timer as wake up source
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  //Print the wakeup reason for ESP32
  serialTimeStampPrefix();
  print_wakeup_reason(wakeup_reason);

  // if wakeup reason was timer then add seconds ticker signal to wake up source and go back to sleep
  if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // add ext0 RTC seconds ticker as wake up source
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SQW_INT_PIN,1); //Wake up at: 1 = High, 0 = Low
    //Go to sleep now
    serialTimeStampPrefix();
    Serial.println("Go To Light Sleep until seconds tick or button press");
    //esp_deep_sleep_start();
    Serial.flush();
    // go to light sleep
    esp_light_sleep_start();

    // On WAKEUP disable EXT0 as wake up source
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);

    wakeup_reason = esp_sleep_get_wakeup_cause();
    // if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    //   turnBacklightOn();

    //Print the wakeup reason for ESP32
    serialTimeStampPrefix();
    print_wakeup_reason(wakeup_reason);
  }
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(esp_sleep_wakeup_cause_t &wakeup_reason){
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println(F("Wakeup by ext signal RTC_IO - SECONDS TICK")); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println(F("Wakeup by ext signal RTC_CNTL - BUTTON PRESS")); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println(F("Wakeup caused by TIMER")); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println(F("Wakeup caused by touchpad")); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println(F("Wakeup caused by ULP program")); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
#endif










void rtc_clock_initialize() {

  /* INITIALIZE RTC */

  // initialize Wire lib
  URTCLIB_WIRE.begin();

  // set rtc model
  rtc.set_model(URTCLIB_MODEL_DS3231);

  // get data from DS3231
  rtc.refresh();

  // to setup DS3231 - run this only for the first time during setup, thereafter set back to false and flash the code again so this is never run again
  if (0) {
    // Set Oscillator to use VBAT when VCC turns off
    if(rtc.enableBattery())
      Serial.println(F("Enable Battery Success"));
    else
      Serial.println(F("Enable Battery UNSUCCESSFUL!"));

    // disable SQ wave out
    rtc.disable32KOut();
    Serial.println(F("disable32KOut() done"));

    // stop sq wave on sqw pin
    rtc.sqwgSetMode(URTCLIB_SQWG_OFF_1);
    Serial.println(F("stop sq wave on sqw pin. Mode set: URTCLIB_SQWG_OFF_1"));

    // clear alarms flags
    rtc.alarmClearFlag(URTCLIB_ALARM_1);
    rtc.alarmClearFlag(URTCLIB_ALARM_2);
    Serial.println(F("alarmClearFlag() done"));

    // disable alarms
    rtc.alarmDisable(URTCLIB_ALARM_1);
    rtc.alarmDisable(URTCLIB_ALARM_2);
    Serial.println(F("alarmDisable() done"));

    // Set current time and date
    Serial.println();
    Serial.println(F("Waiting for input from user to set time."));
    Serial.println(F("Provide a keyboard input when set time is equal to real world time..."));
    while (Serial.available() == 0) {};
    // Only used once, then disabled
    rtc.set(0, 30, 2, 6, 26, 1, 24);
    //  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
    Serial.println(F("Time set"));
    serial_input_flush();
  }

  // Check if time is up to date
  Serial.print(F("Lost power status: "));
  if (rtc.lostPower()) {
    Serial.println(F("POWER FAILED. Clearing flag..."));
    rtc.lostPowerClear();
  }
  else
    Serial.println(F("POWER OK"));

  // Check whether OSC is set to use VBAT or not
  if (rtc.getEOSCFlag())
    Serial.println(F("Oscillator will NOT use VBAT when VCC cuts off. Time will not increment without VCC!"));
  else
    Serial.println(F("Oscillator will use VBAT if VCC cuts off."));

  // we won't use RTC for alarm
  rtc.alarmDisable(URTCLIB_ALARM_1);
  rtc.alarmDisable(URTCLIB_ALARM_2);

  // make second equal to rtc second (-1 just to make first rtc refresh come later than when it should, as there is no time sync initially)
  second = rtc.second() - 1;
}

void serialTimeStampPrefix() {
  Serial.print(F("("));
  Serial.print(millis());
  if(second < 10) Serial.print('0');
  Serial.print(second);
  Serial.print(F(") - "));
  Serial.flush();
}

void serial_input_flush() {
  while (true) {
    delay(20);  // give data a chance to arrive
    if (Serial.available()) {
      // we received something, get all of it and discard it
      while (Serial.available())
        Serial.read();
      continue;  // stay in the main loop
    } else
      break;  // nothing arrived for 20 ms
  }
}

void processSerialInput() {
  // take user input
  char input = Serial.read();
  serial_input_flush();
  // acceptable user input
  Serial.print(F("User input: "));
  Serial.println(input);
  // process user input
  switch (input) {
    case 'a':
      Serial.println(F("**** Toggle Alarm ****"));
      alarmOn = !alarmOn;
      Serial.print(F("alarmOn = ")); Serial.println(alarmOn);
      break;
    case 'b':
      {
        Serial.println(F("**** Set Brightness [0-255] ****"));
        while (Serial.available() == 0) {};
        int brightnessVal = Serial.parseInt();
        serial_input_flush();
        display.setBrightness(brightnessVal);
      }
      break;
    case 'd':  //disable battery
      {
        Serial.println(F("Disable Battery"));
        bool ret = rtc.disableBattery();
        if (ret) Serial.println(F("Disable Battery Success"));
        else Serial.println(F("Could not Disable Battery!"));
      }
      break;
    case 'e':  //enable battery
      {
        Serial.println(F("Enable Battery"));
        bool ret = rtc.enableBattery();
        if (ret) Serial.println(F("Enable Battery Success"));
        else Serial.println(F("Could not Enable Battery!"));
      }
      break;
    case 'g': // good morning
      {
        display.goodMorningScreen();
      }
      break;
    case 'h':
      {
        Serial.println(F("**** Set clock 12/24 hr mode ****"));
        Serial.println(F("Enter 'twelveHrMode' = 0 or 1"));
        while (Serial.available() == 0) {};
        unsigned int clockModeInp = Serial.parseInt();
        Serial.println(clockModeInp);
        serial_input_flush();
        rtc.set_12hour_mode((bool)clockModeInp);
        // print RTC Date Time and Alarm
        refreshRtcTime = true;
      }
      break;
    case 's':
      {
        Serial.println(F("**** Screensaver ****"));
        display.screensaverControl(!display.screensaver);
      }
      break;
    default:
      Serial.println(F("Unrecognized user input"));
  }
}