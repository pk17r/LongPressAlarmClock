#include "rgb_display.h"
#include "alarm_clock.h"
#include "wifi_stuff.h"
#include "eeprom.h"
#include "rtc.h"
#include "touchscreen.h"

/*!
    @brief  Draw a 565 RGB image at the specified (x,y) position using monochrome 8-bit image.
            Converts each bitmap rows into 16-bit RGB buffer and sends over SPI.
            Adapted from Adafruit_SPITFT.cpp drawRGBBitmap function which is itself
            adapted from https://github.com/PaulStoffregen/ILI9341_t3
            by Marc MERLIN. See examples/pictureEmbed to use this.
            Handles its own transaction and edge clipping/rejection.
    @param  x        Top left corner horizontal coordinate.
    @param  y        Top left corner vertical coordinate.
    @param  bitmap   Pointer to 8-bit array of monochrome image
    @param  w        Width of bitmap in pixels.
    @param  h        Height of bitmap in pixels.
*/
void RGBDisplay::FastDrawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
  int16_t x2, y2;                 // Lower-right coord
  if ((x >= kTftWidth) ||            // Off-edge right
      (y >= kTftHeight) ||           // " top
      ((x2 = (x + w - 1)) < 0) || // " left
      ((y2 = (y + h - 1)) < 0))
    return; // " bottom

  // elapsedMillis timer1;

  int bx1 = 0, by1 = 0, // Clipped top-left within bitmap
      saveW = w,            // Save original bitmap width value
      saveH = h;
  if (x < 0) {              // Clip left
    w += x;
    bx1 = -x;
    x = 0;
  }
  if (y < 0) { // Clip top
    h += y;
    by1 = -y;
    y = 0;
  }
  if (x2 >= kTftWidth)
    w = kTftWidth - x; // Clip right
  if (y2 >= kTftHeight)
    h = kTftHeight - y; // Clip bottom

  int16_t jLim = min(saveH, h + by1);
  int16_t iLim = min(saveW, w + bx1);

  // new 16 bit buffter of length w to hold 1 row colors
  uint16_t buffer16Bit[w];
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);

  int16_t bitmapWidthBytes = (saveW + 7) >> 3;          // bitmap width in bytes
  for (int16_t j = by1; j < jLim; j++) {
    int bufi = 0;
    int16_t i = bx1;
    uint8_t currentByte = bitmap[j * bitmapWidthBytes + (i >> 3)];
    uint8_t bitIndex = 7 - i % 8;
    while(1) {
      buffer16Bit[bufi] = (((currentByte >> bitIndex)  & 0x01) ? color : bg);
      bufi++;   // next row buffer index
      i++;  // next pixel
      if(i >= iLim)
        break;
      bitIndex = 7 - i % 8; // next bit index
      if(bitIndex == 7)  // new byte
        currentByte = bitmap[j * bitmapWidthBytes + (i >> 3)];
    }
    tft.writePixels(buffer16Bit, w);
  }
  tft.endWrite();
  // Serial.print(" fastDrawBitmapTime "); Serial.print(charSpace); Serial.println(timer1);
}

void RGBDisplay::SetAlarmScreen(bool processUserInput, bool inc_button_pressed, bool dec_button_pressed, bool push_button_pressed) {

  int16_t gap_x = kTftWidth / 11;
  int16_t gap_y = kTftHeight / 9;
  int16_t hr_x = gap_x / 2, min_x = 2.25*gap_x, amPm_x = 4.25*gap_x, onOff_x = 6.5*gap_x, setCancel_x = 9*gap_x;
  int16_t time_y = 6*gap_y, onSet_y = 3.5*gap_y, offCancel_y = 6.5*gap_y;
  int16_t incB_y = time_y - 3*gap_y, decB_y = time_y + gap_y;
  uint16_t onFill = kDisplayColorGreen, offFill = kDisplayColorBlack, borderColor = kDisplayColorCyan;
  uint16_t button_w = 2*gap_x, button_h = 2*gap_y;
  const char onStr[] = "ON", offStr[] = "OFF", setStr[] = "Set";

  if(!processUserInput) {
    // make alarm set page

    tft.fillScreen(kDisplayBackroundColor);

    // set title font
    tft.setFont(&Satisfy_Regular18pt7b);

    char title[] = "Set Alarm";

    // change the text color to the background color
    tft.setTextColor(kDisplayBackroundColor);

    int16_t title_x0, title_y0;
    uint16_t title_w, title_h;
    // get bounds of title on tft display (with background color as this causes a blink)
    tft.getTextBounds(title, 0, 0, &title_x0, &title_y0, &title_w, &title_h);

    int16_t title_x = (kTftWidth - title_w) / 2;
    int16_t title_y = 2*gap_y;

    // change the text color to the Alarm color
    tft.setTextColor(kDisplayAlarmColor);
    tft.setCursor(title_x, title_y);
    tft.print(title);

    // font color inside
    tft.setTextColor(kDisplayColorGreen);

    // print alarm time
    tft.setCursor(hr_x, time_y);
    tft.print(alarm_clock->var_1_);
    tft.setCursor(min_x, time_y);
    if(alarm_clock->var_2_ < 10)
      tft.print('0');
    tft.print(alarm_clock->var_2_);
    tft.setCursor(amPm_x, time_y);
    if(alarm_clock->var_3_is_AM_)
      tft.print(kAmLabel);
    else
      tft.print(kPmLabel);

    // draw increase / dec buttons
    // hour
    DrawTriangleButton(hr_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    DrawTriangleButton(hr_x, decB_y, gap_x, gap_y, false, borderColor, offFill);
    // min
    DrawTriangleButton(min_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    DrawTriangleButton(min_x, decB_y, gap_x, gap_y, false, borderColor, offFill);
    // am / pm
    DrawTriangleButton(amPm_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    DrawTriangleButton(amPm_x, decB_y, gap_x, gap_y, false, borderColor, offFill);

    // ON button
    DrawButton(onOff_x, onSet_y, button_w, button_h, onStr, borderColor, onFill, offFill, alarm_clock->var_4_ON_);
    // OFF button
    DrawButton(onOff_x, offCancel_y, button_w, button_h, offStr, borderColor, onFill, offFill, !alarm_clock->var_4_ON_);
    // Set button
    DrawButton(setCancel_x, onSet_y, button_w, button_h, setStr, borderColor, kDisplayColorOrange, offFill, true);
    // Cancel button
    DrawButton(setCancel_x, offCancel_y, button_w, button_h, cancelStr, borderColor, kDisplayColorOrange, offFill, true);

    // high light text / buttons
    ButtonHighlight(hr_x, time_y - gap_y, gap_x, gap_y, (highlight == kAlarmSetPageHour), 10);
  }
  else {
    // processUserInput

    // userButtonClick : 1 and 2 for Hr increase, dec button respectively; 3,4 min; 5,6 AmPm;
    //    0 = no button clicked
    //    1 = Hr Inc button
    //    2 = Hr Dec button
    //    3 = Min Inc button
    //    4 = Min Dec button
    //    5 = AmPm Inc button
    //    6 = AmPm Dec button
    //    7 = Alarm On button
    //    8 = Alarm Off button
    //    9 = Set button
    //    10 = Cancel button
    int16_t userButtonClick = 0;

    if(!inc_button_pressed && !dec_button_pressed && !push_button_pressed) {
      // touch screen input
      int16_t ts_x = ts->GetTouchedPixel()->x, ts_y = ts->GetTouchedPixel()->y;

      // find if user clicked a button
      if(ts_x < amPm_x + gap_x && ts_y > incB_y) {
        // check for increase or decrease button press
        // font color inside
        if(ts_y < incB_y + gap_y) {
          // increase button
          if(ts_x > hr_x && ts_x < hr_x + gap_x)
            userButtonClick = 1;
          else if(ts_x > min_x && ts_x < min_x + gap_x)
            userButtonClick = 3;
          else if(ts_x > amPm_x && ts_x < amPm_x + gap_x)
            userButtonClick = 5;
        }
        else if(ts_y > decB_y && ts_y < decB_y + gap_y) {
          // decrease button
          if(ts_x > hr_x && ts_x < hr_x + gap_x)
            userButtonClick = 2;
          else if(ts_x > min_x && ts_x < min_x + gap_x)
            userButtonClick = 4;
          else if(ts_x > amPm_x && ts_x < amPm_x + gap_x)
            userButtonClick = 6;
        }
      }
      else if(ts_x > onOff_x && ts_x < onOff_x + button_w && ts_y > onSet_y && ts_y < onSet_y + button_h) 
        userButtonClick = 7;
      else if(ts_x > onOff_x && ts_x < onOff_x + button_w && ts_y > offCancel_y && ts_y < offCancel_y + button_h)
        userButtonClick = 8;
      else if(ts_x > setCancel_x && ts_x < setCancel_x + button_w && ts_y > onSet_y && ts_y < onSet_y + button_h)
        userButtonClick = 9;
      else if(ts_x > setCancel_x && ts_x < setCancel_x + button_w && ts_y > offCancel_y && ts_y < offCancel_y + button_h)
        userButtonClick = 10;
    }
    else {
      // button input
      // userButtonClick : 1 and 2 for Hr increase, dec button respectively; 3,4 min; 5,6 AmPm;
      //    0 = no button clicked
      //    1 = Hr Inc button
      //    2 = Hr Dec button
      //    3 = Min Inc button
      //    4 = Min Dec button
      //    5 = AmPm Inc button
      //    6 = AmPm Dec button
      //    7 = Alarm On button
      //    8 = Alarm Off button
      //    9 = Set button
      //    10 = Cancel button

      if(highlight == kAlarmSetPageHour) {
        if(inc_button_pressed) userButtonClick = 1;
        else if(dec_button_pressed) userButtonClick = 2;
        else highlight = kAlarmSetPageMinute;
      }
      else if(highlight == kAlarmSetPageMinute) {
        if(inc_button_pressed) userButtonClick = 3;
        else if(dec_button_pressed) userButtonClick = 4;
        else highlight = kAlarmSetPageAmPm;
      }
      else if(highlight == kAlarmSetPageAmPm) {
        if(inc_button_pressed) userButtonClick = 5;
        else if(dec_button_pressed) userButtonClick = 6;
        else {
          // if(alarm_clock->var_4_ON_)
            highlight = kAlarmSetPageOn;
          // else
            // highlight = kAlarmSetPageOff;
        }
      }
      else if(highlight == kAlarmSetPageOn || highlight == kAlarmSetPageOff) {
        if(inc_button_pressed) highlight = kAlarmSetPageOn;
        else if(dec_button_pressed) highlight = kAlarmSetPageOff;
        else {
          if(highlight == kAlarmSetPageOn)
            userButtonClick = 7;
          else
            userButtonClick = 8;
          highlight = kAlarmSetPageSet;
        }
      }
      else if(highlight == kAlarmSetPageSet || highlight == kAlarmSetPageCancel) {
        if(inc_button_pressed) highlight = kAlarmSetPageSet;
        else if(dec_button_pressed) highlight = kAlarmSetPageCancel;
        else {
          if(highlight == kAlarmSetPageSet)
            userButtonClick = 9;
          else
            userButtonClick = 10;
        }
      }
    }

    // high light text / buttons
    ButtonHighlight(hr_x, time_y - gap_y, gap_x, gap_y, (highlight == kAlarmSetPageHour), 10);
    ButtonHighlight(min_x, time_y - gap_y, gap_x, gap_y, (highlight == kAlarmSetPageMinute), 10);
    ButtonHighlight(amPm_x, time_y - gap_y, gap_x, gap_y, (highlight == kAlarmSetPageAmPm), 10);
    ButtonHighlight(onOff_x, onSet_y, button_w, button_h, (highlight == kAlarmSetPageOn), 5);
    ButtonHighlight(onOff_x, offCancel_y, button_w, button_h, (highlight == kAlarmSetPageOff), 5);
    ButtonHighlight(setCancel_x, onSet_y, button_w, button_h, (highlight == kAlarmSetPageSet), 5);
    ButtonHighlight(setCancel_x, offCancel_y, button_w, button_h, (highlight == kAlarmSetPageCancel), 5);

    // Process user input
    if(userButtonClick >= 1 && userButtonClick <= 6) {
      // hr min amPm buttons
      // action is increase/decrease
      // blink triangle
      int16_t triangle_x, triangle_y;
      bool isUp;
      switch(userButtonClick) {
        case 1: triangle_x = hr_x; triangle_y = incB_y; isUp = true; break;
        case 2: triangle_x = hr_x; triangle_y = decB_y; isUp = false; break;
        case 3: triangle_x = min_x; triangle_y = incB_y; isUp = true; break;
        case 4: triangle_x = min_x; triangle_y = decB_y; isUp = false; break;
        case 5: triangle_x = amPm_x; triangle_y = incB_y; isUp = true; break;
        case 6: triangle_x = amPm_x; triangle_y = decB_y; isUp = false; break;
      }
      // turn triangle On
      DrawTriangleButton(triangle_x, triangle_y, gap_x, gap_y, isUp, borderColor, onFill);
      // clear old values
      tft.setFont(&Satisfy_Regular18pt7b);
      tft.setCursor(triangle_x, time_y);
      tft.setTextColor(kDisplayBackroundColor);
      if(userButtonClick <= 2)
        tft.print(alarm_clock->var_1_);
      else if(userButtonClick <= 4) {
        if(alarm_clock->var_2_ < 10) tft.print('0');
        tft.print(alarm_clock->var_2_);
      }
      else {
        if(alarm_clock->var_3_is_AM_)
          tft.print(kAmLabel);
        else
          tft.print(kPmLabel);
      }
      // update value
      if(userButtonClick <= 2) {
        if(!isUp) {  // increase hour
          if(alarm_clock->var_1_ < 12)
            alarm_clock->var_1_++;
          else
            alarm_clock->var_1_ = 1;
        }
        else {  // decrease hour
          if(alarm_clock->var_1_ > 1)
            alarm_clock->var_1_--;
          else
            alarm_clock->var_1_ = 12;
        }
      }
      else if(userButtonClick <= 4) {
        if(!isUp) {  // increase min
          if(alarm_clock->var_2_  < 59)
            alarm_clock->var_2_++;
          else
            alarm_clock->var_2_ = 0;
        }
        else {  // decrease min
          if(alarm_clock->var_2_  > 0)
            alarm_clock->var_2_--;
          else
            alarm_clock->var_2_ = 59;
        }
      }
      else  // toggle alarm Am Pm
        alarm_clock->var_3_is_AM_ = !alarm_clock->var_3_is_AM_;
      // print updated value
      tft.setCursor(triangle_x, time_y);
      tft.setTextColor(kDisplayColorGreen);
      if(userButtonClick <= 2)
        tft.print(alarm_clock->var_1_);
      else if(userButtonClick <= 4) {
        if(alarm_clock->var_2_ < 10) tft.print('0');
        tft.print(alarm_clock->var_2_);
      }
      else {
        if(alarm_clock->var_3_is_AM_)
          tft.print(kAmLabel);
        else
          tft.print(kPmLabel);
      }
      // wait a little
      if(userButtonClick != 3 && userButtonClick != 4)  // no extra wait for minutes, to make inc/dec fast
        delay(kUserInputDelayMs);
      // turn triangle Off
      DrawTriangleButton(triangle_x, triangle_y, gap_x, gap_y, isUp, borderColor, offFill);
    }
    else if(userButtonClick == 7 || userButtonClick == 8) {
      // On or Off button pressed
      if((userButtonClick == 7 && !alarm_clock->var_4_ON_) || (userButtonClick == 8 && alarm_clock->var_4_ON_)) {
        // toggle alarm
        alarm_clock->var_4_ON_ = !alarm_clock->var_4_ON_;
        // draw new ON button with push effect
        DrawButton(onOff_x, onSet_y, button_w, button_h, onStr, borderColor, kDisplayAlarmColor, offFill, alarm_clock->var_4_ON_);
        // draw new OFF button
        DrawButton(onOff_x, offCancel_y, button_w, button_h, offStr, borderColor, onFill, offFill, !alarm_clock->var_4_ON_);
        delay(2*kUserInputDelayMs);
        // draw new ON button
        DrawButton(onOff_x, onSet_y, button_w, button_h, onStr, borderColor, onFill, offFill, alarm_clock->var_4_ON_);
      }
      else if(userButtonClick == 7 && alarm_clock->var_4_ON_) {
        // alarm is On but user pressed On button
        // show a little graphic of input taken
        DrawButton(onOff_x, onSet_y, button_w, button_h, onStr, borderColor, kDisplayAlarmColor, offFill, alarm_clock->var_4_ON_);
        delay(2*kUserInputDelayMs);
        DrawButton(onOff_x, onSet_y, button_w, button_h, onStr, borderColor, onFill, offFill, alarm_clock->var_4_ON_);
      }
      else if(userButtonClick == 8 && !alarm_clock->var_4_ON_) {
        // alarm is Off but user pressed Off button
        // show a little graphic of input taken
        DrawButton(onOff_x, offCancel_y, button_w, button_h, offStr, borderColor, kDisplayAlarmColor, offFill, !alarm_clock->var_4_ON_);
        delay(2*kUserInputDelayMs);
        DrawButton(onOff_x, offCancel_y, button_w, button_h, offStr, borderColor, onFill, offFill, !alarm_clock->var_4_ON_);
      }
    }
    else if(userButtonClick == 9 || userButtonClick == 10) {
      // set or cancel button pressed
      if(userButtonClick == 9) {  // set button pressed
        // show a little graphic of Set Button Press
        DrawButton(setCancel_x, onSet_y, button_w, button_h, setStr, borderColor, kDisplayColorRed, offFill, true);
        // save Set Alarm Page values
        alarm_clock->SaveAlarm();
      }
      else  // show a little graphic of Cancel button Press
        DrawButton(setCancel_x, offCancel_y, button_w, button_h, cancelStr, borderColor, kDisplayColorRed, offFill, true);
      // wait a little
      delay(2*kUserInputDelayMs);
      // go back to main page
      SetPage(kMainPage);
    }
    
  }
}

void RGBDisplay::DrawButton(int16_t x, int16_t y, uint16_t w, uint16_t h, const char* label, uint16_t borderColor, uint16_t onFill, uint16_t offFill, bool isOn) {
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor((isOn ? onFill : offFill));
  int16_t title_x0, title_y0;
  uint16_t title_w, title_h;
  // get bounds of title on tft display (with background color as this causes a blink)
  tft.getTextBounds(label, x, y + h, &title_x0, &title_y0, &title_w, &title_h);
  // make button
  tft.fillRoundRect(x, y, w, h, kRadiusButtonRoundRect, (isOn ? onFill : offFill));
  tft.drawRoundRect(x, y, w, h, kRadiusButtonRoundRect, borderColor);
  tft.setTextColor((isOn ? offFill : onFill));
  title_x0 = x + (w - title_w) / 2;
  title_y0 = y + h / 2 + title_h / 2;
  tft.setCursor(title_x0, title_y0);
  tft.print(label);
}

void RGBDisplay::DrawTriangleButton(int16_t x, int16_t y, uint16_t w, uint16_t h, bool isUp, uint16_t borderColor, uint16_t fillColor) {
  int16_t x1, y1, x2, y2, x3, y3;
  if(isUp) {
    x1 = x + w / 2;
    y1 = y;
    x2 = x;
    y2 = y + h;
    x3 = x + w;
    y3 = y + h;
  }
  else {
    x1 = x;
    y1 = y;
    x2 = x + w / 2;
    y2 = y + h;
    x3 = x + w;
    y3 = y;
  }
  // make button
  tft.fillTriangle(x1, y1, x2, y2, x3, y3, fillColor);
  tft.drawTriangle(x1, y1, x2, y2, x3, y3, borderColor);
}

void RGBDisplay::SettingsPage(bool inc_alarm_long_press_secs, bool dec_alarm_long_press_secs) {

  if(!inc_alarm_long_press_secs && !dec_alarm_long_press_secs) {
    tft.fillScreen(kDisplayBackroundColor);
    tft.setTextColor(kDisplayColorYellow);
    tft.setFont(&FreeSans12pt7b);
    tft.setCursor(10, 40);
    tft.print("Settings:");

    // Update WiFi Details button
    DrawButton(kWiFiSettingsButtonX1, kWiFiSettingsButtonY1, kWiFiSettingsButtonW, kWiFiSettingsButtonH, wifiSettingsStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

    // Update Weather and Location button
    DrawButton(kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, weatherStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(kDisplayColorYellow);
    tft.setCursor(10, kAlarmLongPressSecondsY0 - 20);
    tft.print("Alarm Long");
    tft.setCursor(10, kAlarmLongPressSecondsY0);
    tft.print("Press Seconds:");
    tft.setCursor(kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY0);
    tft.print(alarm_clock->alarm_long_press_seconds_);
    DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - 2 * kAlarmLongPressSecondsTriangleButtonsSize - 2, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, true, kDisplayColorCyan, kDisplayBackroundColor);
    DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, false, kDisplayColorCyan, kDisplayBackroundColor);
    // Set button
    DrawButton(kAlarmLongPressSecondsSetButtonX1, kAlarmLongPressSecondsSetButtonY1, kAlarmLongPressSecondsSetButtonW, kAlarmLongPressSecondsSetButtonH, setStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(kDisplayColorYellow);
    tft.setCursor(10, kScreensaverButtonY1 + kScreensaverButtonH * 3 / 4);
    tft.print("Run:");

    // Start Screensaver Button
    DrawButton(kScreensaverButtonX1, kScreensaverButtonY1, kScreensaverButtonW, kScreensaverButtonH, screensaverStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

    // Cancel button
    DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);
  }
  else {
    // clear current alarm long press seconds
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(kDisplayBackroundColor);
    tft.setCursor(kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY0);
    tft.print(alarm_clock->alarm_long_press_seconds_);
    // change seconds
    if(inc_alarm_long_press_secs && alarm_clock->alarm_long_press_seconds_ < 30)
      alarm_clock->alarm_long_press_seconds_ += 5;
    else if(dec_alarm_long_press_secs && alarm_clock->alarm_long_press_seconds_ > 5)
      alarm_clock->alarm_long_press_seconds_ -= 5;
    // draw new seconds
    tft.setTextColor(kDisplayColorYellow);
    tft.setCursor(kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY0);
    tft.print(alarm_clock->alarm_long_press_seconds_);
    // show effect of user input
    if(inc_alarm_long_press_secs) {
      DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - 2 * kAlarmLongPressSecondsTriangleButtonsSize - 2, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, true, kDisplayColorCyan, kDisplayColorGreen);
      delay(kUserInputDelayMs);
      DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - 2 * kAlarmLongPressSecondsTriangleButtonsSize - 2, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, true, kDisplayColorCyan, kDisplayBackroundColor);
    }
    if(dec_alarm_long_press_secs) {
      DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, false, kDisplayColorCyan, kDisplayColorGreen);
      delay(kUserInputDelayMs);
      DrawTriangleButton(kAlarmLongPressSecondsX0 + kAlarmLongPressSecondsW, kAlarmLongPressSecondsY0 - kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsTriangleButtonsSize, false, kDisplayColorCyan, kDisplayBackroundColor);
    }
    InstantHighlightResponse(/* color_button = */ kCursorNoSelection);
  }
}

void RGBDisplay::SoftApInputsPage() {

  tft.fillScreen(kDisplayBackroundColor);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.setCursor(10, 20);
  tft.print("Connect to Created WiFi");
  tft.setCursor(10, 40);
  tft.print("using mobile/computer.");
  tft.setCursor(10, 60);
  tft.print("WiFi SSID:");

  tft.setCursor(10, 90);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(kDisplayColorGreen);
  tft.print(softApSsid);

  tft.setCursor(10, 120);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.print("Open web browser and in");
  tft.setCursor(10, 140);
  tft.print("address bar enter:");

  tft.setCursor(10, 170);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(kDisplayColorGreen);
  tft.print(wifi_stuff->soft_AP_IP.c_str());

  tft.setCursor(10, 200);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.print("Set your 2.4GHz");
  tft.setCursor(10, 220);
  tft.print("WiFi details.");

  // Save button
  DrawButton(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, saveStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Cancel button
  DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);
}

void RGBDisplay::WiFiSettingsPage() {

  tft.fillScreen(kDisplayBackroundColor);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(10, 20);
  tft.print("WiFi:");
  tft.setFont(&FreeMono9pt7b);
  tft.setCursor(10, 40);
  tft.print("ssid: ");
  tft.print(wifi_stuff->wifi_ssid_.c_str());
  tft.setCursor(10, 60);
  tft.print("pass: ");
  int i = 0;
  while(i <= kWifiSsidPasswordLengthMax) {
    char c = wifi_stuff->wifi_password_[i];
    if(i % 4 == 0)
      tft.print(c);
    else
      tft.print('*');
    i++;
    if(c == wifi_stuff->wifi_password_.length())
     break;
  }

  // Set WiFi SSID and Passwd button
  DrawButton(kSetWiFiButtonX1, kSetWiFiButtonY1, kSetWiFiButtonW, kSetWiFiButtonH, setWiFiStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Connect To WiFi
  DrawButton(kConnectWiFiButtonX1, kConnectWiFiButtonY1, kConnectWiFiButtonW, kConnectWiFiButtonH, connectWiFiStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Disconnect WiFi button
  DrawButton(kDisconnectWiFiButtonX1, kDisconnectWiFiButtonY1, kDisconnectWiFiButtonW, kDisconnectWiFiButtonH, disconnectWiFiStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  int16_t title_y0 = 50;
  int16_t w_x0 = 5, s_y0 = title_y0 + 48;

  if(wifi_stuff->wifi_connected_) {
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(kDisplayColorOrange);
    tft.setCursor(w_x0, s_y0 + 50);
    tft.print("WiFi Connected!");
  }
  else {
    tft.setFont(&FreeMono9pt7b);
    tft.setTextColor(kDisplayColorOrange);
    tft.setCursor(w_x0, s_y0 + 40);
    tft.print("Not connected.");
    tft.setCursor(w_x0, s_y0 + 65);
    tft.print("Click CONNECT Button,");
    tft.setCursor(w_x0, s_y0 + 90);
    tft.print("Wait a few seconds..");
  }

  // Cancel button
  DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);
}

void RGBDisplay::WeatherSettingsPage() {
  tft.fillScreen(kDisplayBackroundColor);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(10, 20);
  tft.print("Weather Location:");
  tft.setFont(&FreeMono9pt7b);
  tft.setCursor(10, 40);
  tft.print("ZIP Code: ");
  tft.print(wifi_stuff->location_zip_code_);
  tft.setCursor(10, 60);
  tft.print("Country Code: ");
  tft.print(wifi_stuff->location_country_code_.c_str());
  tft.setCursor(10, 80);
  tft.print("Units: "); tft.print(wifi_stuff->weather_units_metric_not_imperial_ ? metricUnitStr : imperialUnitStr);
  
  // Set Location button
  DrawButton(kSetLocationButtonX1, kSetLocationButtonY1, kSetLocationButtonW, kSetLocationButtonH, setStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Toggle Units Metric/Imperial button
  DrawButton(kUnitsButtonX1, kUnitsButtonY1, kUnitsButtonW, kUnitsButtonH, unitsStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Get WEATHER Info button
  DrawButton(kFetchWeatherButtonX1, kFetchWeatherButtonY1, kFetchWeatherButtonW, kFetchWeatherButtonH, fetchStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Update TIME button
  DrawButton(kUpdateTimeButtonX1, kUpdateTimeButtonY1, kUpdateTimeButtonW, kUpdateTimeButtonH, updateTimeStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  DisplayWeatherInfo();

  // Cancel button
  DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);
}

void RGBDisplay::LocationInputsLocalServerPage() {

  tft.fillScreen(kDisplayBackroundColor);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.setCursor(10, 20);
  tft.print("Connect to this WiFi");
  tft.setCursor(10, 40);
  tft.print("using mobile/computer.");
  tft.setCursor(10, 60);
  tft.print("WiFi SSID:");

  tft.setCursor(10, 90);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(kDisplayColorGreen);
  tft.print(wifi_stuff->wifi_ssid_.c_str());

  tft.setCursor(10, 120);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.print("Open web browser and in");
  tft.setCursor(10, 140);
  tft.print("address bar enter:");

  tft.setCursor(10, 170);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(kDisplayColorGreen);
  tft.print(wifi_stuff->soft_AP_IP.c_str());

  tft.setCursor(10, 200);
  tft.setTextColor(kDisplayColorYellow);
  tft.setFont(&FreeMono9pt7b);
  tft.print("Set Location");
  tft.setCursor(10, 220);
  tft.print("details.");

  // Save button
  DrawButton(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, saveStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);

  // Cancel button
  DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorOrange, kDisplayColorBlack, true);
}

void RGBDisplay::DisplayWeatherInfo() {

  int16_t title_y0 = 50;
  int16_t w_x0 = 5, s_y0 = title_y0 + 48;

  // show today's weather
  if(wifi_stuff->got_weather_info_) {
    // tft.setFont(&FreeMonoBold9pt7b);
    tft.setFont(&FreeSans12pt7b);
    tft.setCursor(200, s_y0 + 40);
    tft.setTextColor(kDisplayColorOrange);
    tft.print(wifi_stuff->city_.c_str());
    tft.setCursor(w_x0, s_y0 + 65);
    tft.print(wifi_stuff->weather_main_.c_str()); tft.print(" : "); tft.print(wifi_stuff->weather_description_.c_str());
    tft.setFont(&FreeMono9pt7b);
    tft.setCursor(w_x0, s_y0 + 85);
    tft.print("Temp: "); tft.print(wifi_stuff->weather_temp_.c_str()); tft.print("  Feels: "); tft.print(wifi_stuff->weather_temp_feels_like_.c_str());
    tft.setCursor(w_x0, s_y0 + 105);
    tft.print("Max : "); tft.print(wifi_stuff->weather_temp_max_.c_str()); tft.print("  Min: "); tft.print(wifi_stuff->weather_temp_min_.c_str());
    tft.setCursor(w_x0, s_y0 + 125);
    tft.print("Wind: "); tft.print(wifi_stuff->weather_wind_speed_.c_str()); tft.print(" Humidity: "); tft.print(wifi_stuff->weather_humidity_.c_str());
  }
  else {
    tft.setTextColor(kDisplayColorYellow);
    tft.setFont(&FreeMono9pt7b);
    tft.setCursor(10, 150);
    tft.print("Could not fetch");
    tft.setCursor(10, 180);
    tft.print("Weather info!");
  }
}

void RGBDisplay::AlarmTriggeredScreen(bool firstTime, int8_t buttonPressSecondsCounter) {

  int16_t title_x0 = 30, title_y0 = 40;
  int16_t s_x0 = 230, s_y0 = title_y0 + 48;
  
  if(firstTime) {

    tft.fillScreen(kDisplayBackroundColor);
    tft.setFont(&Satisfy_Regular24pt7b);
    tft.setTextColor(kDisplayColorYellow);
    tft.setCursor(title_x0, title_y0);
    tft.print("WAKE UP!");
    tft.setTextColor(kDisplayColorCyan);
    char press_button_text1[] = "To turn off Alarm,";
    char press_button_text2[] = "press button for:";
    tft.setFont(&FreeMono9pt7b);
    tft.setCursor(10, s_y0 - 18);
    tft.print(press_button_text1);
    tft.setCursor(10, s_y0);
    tft.print(press_button_text2);
    
    // show today's date
    tft.setCursor(20, s_y0 + 35);
    tft.setFont(&Satisfy_Regular18pt7b);
    tft.setTextColor(kDisplayDateColor);
    tft.print(new_display_data_.date_str);

    // show today's weather
    DisplayWeatherInfo();
  }

  char timer_str[4];
  int charIndex = 0;
  if(buttonPressSecondsCounter > 9) {
    timer_str[charIndex] = (char)(buttonPressSecondsCounter / 10 + 48); charIndex++;
  }
  timer_str[charIndex] = (char)(buttonPressSecondsCounter % 10 + 48); charIndex++;
  timer_str[charIndex] = 's'; charIndex++;
  timer_str[charIndex] = '\0';

  // fill rect
  tft.fillRect(s_x0 - 5, s_y0 - 40, 80, 46, kDisplayBackroundColor);

  // set font
  tft.setFont(&Satisfy_Regular24pt7b);
  tft.setTextColor(kDisplayColorYellow);

  // home the cursor
  // uint16_t h = 0, w = 0;
  // int16_t x = 0, y = 0;
  // tft.getTextBounds(timer_str, 10, 150, &x, &y, &w, &h);
  // Serial.print("\nx "); Serial.print(x); Serial.print(" y "); Serial.print(y); Serial.print(" w "); Serial.print(w); Serial.print(" h "); Serial.println(h); 
  tft.setCursor(s_x0, s_y0);
  tft.print(timer_str);
}

void RGBDisplay::Screensaver() {
  // elapsedMillis timer1;
  const int16_t GAP_BAND = 5;
  if(refresh_screensaver_canvas_) {

    // delete created canvas and null the pointer
    if(my_canvas_ != NULL) {
      delete my_canvas_;
      my_canvas_ = NULL;
      // myCanvas.reset(nullptr);
    }
    // alarmClock->serialTimeStampPrefix(); Serial.println("After deleting myCanvas."); Serial.flush();

    // get bounds of HH:MM text on screen
    tft.setFont(&ComingSoon_Regular70pt7b);
    tft.setTextColor(kDisplayBackroundColor);
    tft.getTextBounds(new_display_data_.time_HHMM, 0, 0, &gap_right_x_, &gap_up_y_, &tft_HHMM_w_, &tft_HHMM_h_);

    // get bounds of date string
    uint16_t date_h = 0, date_w = 0;
    int16_t date_gap_x = 0, date_gap_y = 0;
    if(rtc->hour() >= 10)
      tft.setFont(&Satisfy_Regular24pt7b);
    else
      tft.setFont(&Satisfy_Regular18pt7b);
    tft.getTextBounds(new_display_data_.date_str, 0, 0, &date_gap_x, &date_gap_y, &date_w, &date_h);
    
    int16_t date_x0 = GAP_BAND - date_gap_x;
    int16_t alarm_icon_w = (new_display_data_.alarm_ON ? kBellSmallWidth : kBellFallenSmallWidth);
    int16_t alarm_icon_h = (new_display_data_.alarm_ON ? kBellSmallHeight : kBellFallenSmallHeight);
    uint16_t date_row_w = date_w + 2 * GAP_BAND + alarm_icon_w;
    screensaver_w_ = max(tft_HHMM_w_ + 5 * GAP_BAND, date_row_w + 5 * GAP_BAND);
    screensaver_h_ = tft_HHMM_h_ + max((int)date_h, (int)alarm_icon_h) + 3*GAP_BAND;
    // middle both rows
    tft_HHMM_x0_ = (screensaver_w_ - tft_HHMM_w_) / 2 - gap_right_x_;
    date_x0 = (screensaver_w_ - date_row_w) / 2 - date_gap_x;
    
    // create canvas
    // myCanvas = new GFXcanvas16(screensaver_w, screensaver_h);
    my_canvas_ = new GFXcanvas1(screensaver_w_, screensaver_h_);

    // alarmClock->serialTimeStampPrefix(); Serial.println("After creating canvas."); Serial.flush();

    my_canvas_->setTextWrap(false);
    my_canvas_->fillScreen(kDisplayBackroundColor);

    // alarmClock->serialTimeStampPrefix(); Serial.println("After clearing canvas."); Serial.flush();

    // picknew random color
    PickNewRandomColor();
    uint16_t randomColor = kColorPickerWheel[current_random_color_index_];

    // print HH:MM
    my_canvas_->setFont(&ComingSoon_Regular70pt7b);
    my_canvas_->setTextColor(randomColor);
    my_canvas_->setCursor(tft_HHMM_x0_ + GAP_BAND, GAP_BAND - gap_up_y_);
    my_canvas_->print(new_display_data_.time_HHMM);

    // print date string
    if(rtc->hour() >= 10)
      my_canvas_->setFont(&Satisfy_Regular24pt7b);
    else
      my_canvas_->setFont(&Satisfy_Regular18pt7b);
    my_canvas_->setTextColor(randomColor);
    my_canvas_->setCursor(date_x0 + GAP_BAND, screensaver_h_ - 3 * GAP_BAND);
    my_canvas_->print(new_display_data_.date_str);

    // draw bell
    my_canvas_->drawBitmap(my_canvas_->getCursorX() + 2*GAP_BAND, screensaver_h_ - alarm_icon_h - GAP_BAND, (new_display_data_.alarm_ON ? kBellSmallBitmap : kBellFallenSmallBitmap), alarm_icon_w, alarm_icon_h, randomColor);

    // get visual bounds of created canvas and time string
    // myCanvas->drawRect(tft_HHMM_x0 + GAP_BAND, GAP_BAND, tft_HHMM_w, tft_HHMM_h, Display_Color_Green);  // time border
    // myCanvas->drawRect(date_x0 + GAP_BAND, screensaver_h + date_gap_y - 2 * GAP_BAND, date_row_w, date_h, Display_Color_Cyan);  // date row border
    if(show_colored_edge_screensaver_)
      my_canvas_->drawRect(0,0, screensaver_w_, screensaver_h_, kDisplayColorWhite);  // canvas border

    if(rtc->year() < 2024) {
      // RTC Time is not Set!
      my_canvas_->fillRect(0, 0, screensaver_w_, kTimeRowY0, kDisplayBackroundColor);
      my_canvas_->drawRect(0, 0, screensaver_w_, kTimeRowY0, kDisplayTimeColor);
      my_canvas_->setFont(&FreeSans18pt7b);
      my_canvas_->setCursor(kDisplayTextGap, kTimeRowY0 * 0.5 - 5);
      my_canvas_->print("Clock Power Lost!");
      my_canvas_->setFont(&FreeSans12pt7b);
      my_canvas_->setCursor(kDisplayTextGap, kTimeRowY0 - 10);
      my_canvas_->print("Fetching Time using WiFi..");
    }

    // stop refreshing canvas until time change or if it hits top or bottom screen edges
    refresh_screensaver_canvas_ = false;

    // alarmClock->serialTimeStampPrefix(); Serial.println("After completing canvas."); Serial.flush();
  }
  else {

    // move the time text on screen
    const int16_t adder = 1;
    screensaver_x1_ += (screensaver_move_right_ ? adder : -adder);
    screensaver_y1_ += (screensaver_move_down_ ? adder : -adder);

    // set direction on hitting any edge
    // left and right edge - only change direction
    if(screensaver_x1_ + 2* GAP_BAND <= 0) {   // left edge
      if(!screensaver_move_right_) {
        screensaver_move_right_ = true;
        if(rtc->hour() < 10)
          refresh_screensaver_canvas_ = true;
      }
    }
    else if(screensaver_x1_ + screensaver_w_ - 2 * GAP_BAND >= kTftWidth) {    // right edge
      if(!fly_screensaver_horizontally_) {
        if(screensaver_move_right_) {
          screensaver_move_right_ = false;
          if(rtc->hour() < 10)
            refresh_screensaver_canvas_ = true;
        }
      }
      else {  // fly through right edge and apprear back on left edge
        if(screensaver_x1_ > kTftWidth)
          screensaver_x1_ = - kTftWidth;
      }
    }
    // top and bottom edge - when hit change color as well
    if(screensaver_y1_ + GAP_BAND <= 0)  {   // top edge
      if(!screensaver_move_down_) {
        screensaver_move_down_ = true;
        refresh_screensaver_canvas_ = true;
      }
    }
    else if(screensaver_y1_ + screensaver_h_ - GAP_BAND >= kTftHeight)  {   // bottom edge
      if(screensaver_move_down_) {
        screensaver_move_down_ = false;
        refresh_screensaver_canvas_ = true;
      }
    }
  }

  // paste the canvas on screen
  // unsigned long time1 = timer1; timer1 = 0;
  // tft.drawRGBBitmap(screensaver_x1, screensaver_y1, myCanvas->getBuffer(), screensaver_w, screensaver_h); // Copy to screen
  // tft.drawBitmap(screensaver_x1, screensaver_y1, myCanvas->getBuffer(), screensaver_w, screensaver_h, colorPickerWheelBright[currentRandomColorIndex], Display_Backround_Color); // Copy to screen
  FastDrawBitmap(screensaver_x1_, screensaver_y1_, my_canvas_->getBuffer(), screensaver_w_, screensaver_h_, kColorPickerWheel[current_random_color_index_], kDisplayBackroundColor);
  // unsigned long time2 = timer1;
  // Serial.print("Screensaver canvas and drawRGBBitmap times (ms): "); Serial.print(time1); Serial.print(' '); Serial.println(time2);
}

void RGBDisplay::PickNewRandomColor() {
  int newIndex = current_random_color_index_;
  while(newIndex == current_random_color_index_)
    newIndex = random(0, kColorPickerWheelSize - 1);
  current_random_color_index_ = newIndex;
  // Serial.println(currentRandomColorIndex);
}

void RGBDisplay::DisplayTimeUpdate() {

  bool isThisTheFirstTime = strcmp(displayed_data_.time_SS, "") == 0;
  if(redraw_display_) {
    tft.fillScreen(kDisplayBackroundColor);
    isThisTheFirstTime = true;
  }

  // initial gap if single digit hour
  const int16_t SINGLE_DIGIT_HOUR_GAP = 30;
  int16_t hh_gap_x = (rtc->hour() >= 10 ? 0 : SINGLE_DIGIT_HOUR_GAP);

  if(1) {   // CODE USES CANVAS AND ALWAYS PUTS HH:MM:SS AmPm on it every second

    // delete canvas if it exists
    if(my_canvas_ != NULL) {
      delete my_canvas_;
      my_canvas_ = NULL;
      // myCanvas.reset(nullptr);
    }

    // create new canvas for time row
    // myCanvas = std::unique_ptr<GFXcanvas16>(new GFXcanvas16(TFT_WIDTH, TIME_ROW_Y0 + 6));
    my_canvas_ = new GFXcanvas1(kTftWidth, kTimeRowY0 + 6);
    my_canvas_->fillScreen(kDisplayBackroundColor);
    my_canvas_->setTextWrap(false);

    // HH:MM

    // set font
    my_canvas_->setFont(&FreeSansBold48pt7b);

    // home the cursor
    my_canvas_->setCursor(kTimeRowX0 + hh_gap_x, kTimeRowY0);

    // change the text color to foreground color
    my_canvas_->setTextColor(kDisplayTimeColor);

    // draw the new time value
    my_canvas_->print(new_display_data_.time_HHMM);
    // tft.setTextSize(1);
    // delay(2000);

    // and remember the new value
    strcpy(displayed_data_.time_HHMM, new_display_data_.time_HHMM);


    // AM/PM

    int16_t x0_pos = my_canvas_->getCursorX();

    // set font
    my_canvas_->setFont(&FreeSans18pt7b);

    // draw new AM/PM
    if(new_display_data_._12_hour_mode) {

      // home the cursor
      my_canvas_->setCursor(x0_pos + kDisplayTextGap, kAM_PM_row_Y0);
      // Serial.print("tft_AmPm_x0 "); Serial.print(tft_AmPm_x0); Serial.print(" y0 "); Serial.print(tft_AmPm_y0); Serial.print(" tft.getCursorX() "); Serial.print(tft.getCursorX()); Serial.print(" tft.getCursorY() "); Serial.println(tft.getCursorY()); 

      // draw the new time value
      if(new_display_data_.pm_not_am)
        my_canvas_->print(kPmLabel);
      else
        my_canvas_->print(kAmLabel);
    }

    // and remember the new value
    displayed_data_._12_hour_mode = new_display_data_._12_hour_mode;
    displayed_data_.pm_not_am = new_display_data_.pm_not_am;


    // :SS

    // home the cursor
    my_canvas_->setCursor(x0_pos + kDisplayTextGap, kTimeRowY0);

    // draw the new time value
    my_canvas_->print(new_display_data_.time_SS);

    // and remember the new value
    strcpy(displayed_data_.time_SS, new_display_data_.time_SS);

    if(rtc->year() < 2024) {
      // RTC Time is not Set!
      my_canvas_->fillRect(0, 0, kTftWidth, kTimeRowY0, kDisplayBackroundColor);
      my_canvas_->drawRect(0, 0, kTftWidth, kTimeRowY0, kDisplayTimeColor);
      my_canvas_->setTextColor(kDisplayTimeColor);
      my_canvas_->setFont(&FreeSans18pt7b);
      my_canvas_->setCursor(kDisplayTextGap, kTimeRowY0 * 0.5 - 5);
      my_canvas_->print("Clock Power Lost!");
      my_canvas_->setFont(&FreeSans12pt7b);
      my_canvas_->setCursor(kDisplayTextGap, kTimeRowY0 - 10);
      my_canvas_->print("Fetching Time using WiFi..");
    }

    // draw canvas to tft   fastDrawBitmap
    // elapsedMillis time1;
    // tft.drawBitmap(0, 0, myCanvas->getBuffer(), TFT_WIDTH, TIME_ROW_Y0 + 6, Display_Time_Color, Display_Backround_Color); // Copy to screen
    FastDrawBitmap(0, 0, my_canvas_->getBuffer(), kTftWidth, kTimeRowY0 + 6, kDisplayTimeColor, kDisplayBackroundColor); // Copy to screen
    // tft.drawRGBBitmap(0, 0, myCanvas->getBuffer(), TFT_WIDTH, TIME_ROW_Y0 + 6); // Copy to screen
    // unsigned long timeA = time1;
    // Serial.println();
    // Serial.print("Time to run tft.drawRGBBitmap (ms) = "); Serial.println(timeA);

    // delete created canvas and null the pointer
    delete my_canvas_;
    my_canvas_ = NULL;
    // myCanvas.reset(nullptr);

  }
  else {    // CODE THAT CHECKS AND UPDATES ONLY CHANGES ON SCREEN HH:MM :SS AmPm

    // if new minute has come then clear the full time row and recreate it
    // GFXcanvas16* canvasPtr;
    if(rtc->second() == 0) {
      // canvasPtr = GFXcanvas16(TFT_WIDTH, TIME_ROW_Y0 + gap_up_y + tft_HHMM_h);
      // canvasPtr->fillScreen(Display_Backround_Color);
      tft.fillRect(0, 0, kTftWidth, kTimeRowY0 + gap_up_y_ + tft_HHMM_h_, kDisplayBackroundColor);
    }

    // HH:MM string and AM/PM string
    if (rtc->second() == 0 || strcmp(new_display_data_.time_HHMM, displayed_data_.time_HHMM) != 0 || redraw_display_) {

      // HH:MM

      // set font
      // tft.setTextSize(1);
      tft.setFont(&FreeSansBold48pt7b);

      // change the text color to the background color
      tft.setTextColor(kDisplayBackroundColor);

      // clear old time if it was there
      if(rtc->second() != 0 && !isThisTheFirstTime) {
        // home the cursor to currently displayed text location
        if(rtc->hour() == 10 && rtc->minute() == 0 && rtc->second() == 0)  // handle special case of moving from single digit hour to 2 digit hour while clearing old value
          tft.setCursor(kTimeRowX0 + SINGLE_DIGIT_HOUR_GAP, kTimeRowY0);
        else
          tft.setCursor(kTimeRowX0 + hh_gap_x, kTimeRowY0);

        // redraw the old value to erase
        tft.print(displayed_data_.time_HHMM);
        // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
      }

      // get bounds of new HH:MM string on tft display (with background color as this causes a blink)
      tft.getTextBounds(new_display_data_.time_HHMM, 0, 0, &gap_right_x_, &gap_up_y_, &tft_HHMM_w_, &tft_HHMM_h_);
      // Serial.print("gap_right_x "); Serial.print(gap_right_x); Serial.print(" gap_up_y "); Serial.print(gap_up_y); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 

      // home the cursor
      tft.setCursor(kTimeRowX0 + hh_gap_x, kTimeRowY0);
      // Serial.print("X0 "); Serial.print(TIME_ROW_X0); Serial.print(" Y0 "); Serial.print(TIME_ROW_Y0); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 
      // tft.drawRect(TIME_ROW_X0 + gap_right_x, TIME_ROW_Y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);

      // change the text color to foreground color
      tft.setTextColor(kDisplayTimeColor);

      // draw the new time value
      tft.print(new_display_data_.time_HHMM);
      // tft.setTextSize(1);
      // delay(2000);

      // and remember the new value
      strcpy(displayed_data_.time_HHMM, new_display_data_.time_HHMM);

      // AM/PM

      // set font
      tft.setFont(&FreeSans18pt7b);

      // clear old AM/PM
      if(rtc->second() != 0 && !isThisTheFirstTime && displayed_data_._12_hour_mode) {
        // home the cursor
        tft.setCursor(tft_AmPm_x0_, tft_AmPm_y0_);

        // change the text color to the background color
        tft.setTextColor(kDisplayBackroundColor);

        // redraw the old value to erase
        if(displayed_data_.pm_not_am)
          tft.print(kPmLabel);
        else
          tft.print(kAmLabel);
      }

      // draw new AM/PM
      if(new_display_data_._12_hour_mode) {
        // set test location of Am/Pm
        tft_AmPm_x0_ = kTimeRowX0 + hh_gap_x + gap_right_x_ + tft_HHMM_w_ + 2 * kDisplayTextGap;
        tft_AmPm_y0_ = kTimeRowY0 + gap_up_y_ / 2;

        // home the cursor
        tft.setCursor(tft_AmPm_x0_, tft_AmPm_y0_);
        // Serial.print("tft_AmPm_x0 "); Serial.print(tft_AmPm_x0); Serial.print(" y0 "); Serial.print(tft_AmPm_y0); Serial.print(" tft.getCursorX() "); Serial.print(tft.getCursorX()); Serial.print(" tft.getCursorY() "); Serial.println(tft.getCursorY()); 

        // change the text color to the background color
        tft.setTextColor(kDisplayBackroundColor);

        // get bounds of new AM/PM string on tft display (with background color as this causes a blink)
        int16_t tft_AmPm_x1, tft_AmPm_y1;
        uint16_t tft_AmPm_w, tft_AmPm_h;
        tft.getTextBounds((new_display_data_.pm_not_am ? kPmLabel : kAmLabel), tft.getCursorX(), tft.getCursorY(), &tft_AmPm_x1, &tft_AmPm_y1, &tft_AmPm_w, &tft_AmPm_h);
        // Serial.print("AmPm_x1 "); Serial.print(tft_AmPm_x1); Serial.print(" y1 "); Serial.print(tft_AmPm_y1); Serial.print(" w "); Serial.print(tft_AmPm_w); Serial.print(" h "); Serial.println(tft_AmPm_h); 

        // calculate tft_AmPm_y0 to align top with HH:MM
        tft_AmPm_y0_ -= tft_AmPm_y1 - kTimeRowY0 - gap_up_y_;
        // Serial.print("tft_AmPm_y0 "); Serial.println(tft_AmPm_y0);

        // home the cursor
        tft.setCursor(tft_AmPm_x0_, tft_AmPm_y0_);

        // change the text color to foreground color
        tft.setTextColor(kDisplayTimeColor);

        // draw the new time value
        if(new_display_data_.pm_not_am)
          tft.print(kPmLabel);
        else
          tft.print(kAmLabel);
      }

      // and remember the new value
      displayed_data_._12_hour_mode = new_display_data_._12_hour_mode;
      displayed_data_.pm_not_am = new_display_data_.pm_not_am;
    }

    // :SS string
    if (rtc->second() == 0 || strcmp(new_display_data_.time_SS, displayed_data_.time_SS) != 0 || redraw_display_) {
      // set font
      tft.setFont(&FreeSans24pt7b);

      // clear old seconds
      if(rtc->second() != 0 && !isThisTheFirstTime) {
        // change the text color to the background color
        tft.setTextColor(kDisplayBackroundColor);

        // home the cursor
        tft.setCursor(tft_SS_x0_, kTimeRowY0);

        // change the text color to the background color
        tft.setTextColor(kDisplayBackroundColor);

        // redraw the old value to erase
        tft.print(displayed_data_.time_SS);
      }

      // fill new home values
      tft_SS_x0_ = kTimeRowX0 + hh_gap_x + gap_right_x_ + tft_HHMM_w_ + kDisplayTextGap;

      // home the cursor
      tft.setCursor(tft_SS_x0_, kTimeRowY0);

      // change the text color to foreground color
      tft.setTextColor(kDisplayTimeColor);

      // draw the new time value
      tft.print(new_display_data_.time_SS);

      // and remember the new value
      strcpy(displayed_data_.time_SS, new_display_data_.time_SS);
    }
  }

  // date string center aligned
  if (strcmp(new_display_data_.date_str, displayed_data_.date_str) != 0 || redraw_display_) {
    // set font
    tft.setFont(&Satisfy_Regular24pt7b);

    // change the text color to the background color
    tft.setTextColor(kDisplayBackroundColor);

    // clear old data
    if(!isThisTheFirstTime) {
      // yes! home the cursor
      tft.setCursor(date_row_x0_, kDateRow_Y0);

      // redraw the old value to erase
      tft.print(displayed_data_.date_str);
    }

    // home the cursor
    tft.setCursor(date_row_x0_, kDateRow_Y0);

    // record date_row_w to calculate center aligned date_row_x0 value
    int16_t date_row_y1;
    uint16_t date_row_w, date_row_h;
    // get bounds of new dateStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(new_display_data_.date_str, tft.getCursorX(), tft.getCursorY(), &date_row_x0_, &date_row_y1, &date_row_w, &date_row_h);
    date_row_x0_ = (kSettingsGearX1 - date_row_w) / 2;

    // home the cursor
    tft.setCursor(date_row_x0_, kDateRow_Y0);

    // change the text color to foreground color
    tft.setTextColor(kDisplayDateColor);

    // draw the new dateStr value
    tft.print(new_display_data_.date_str);

    // draw settings gear
    tft.drawBitmap(kSettingsGearX1, kSettingsGearY1, kSettingsGearBitmap, kSettingsGearWidth, kSettingsGearHeight, RGB565_Sandy_brown); // Copy to screen

    // and remember the new value
    strcpy(displayed_data_.date_str, new_display_data_.date_str);
  }

  // // settings wheel cursor highlight
  // if(highlight == kMainPageSettingsWheel)
  //   tft.drawRoundRect(kSettingsGearX1 - 1, kSettingsGearY1 - 1, kSettingsGearWidth + 2, kSettingsGearHeight + 2, kRadiusButtonRoundRect, kDisplayColorCyan);
  // else
  //   tft.drawRoundRect(kSettingsGearX1 - 1, kSettingsGearY1 - 1, kSettingsGearWidth + 2, kSettingsGearHeight + 2, kRadiusButtonRoundRect, kDisplayBackroundColor);

  // alarm string center aligned
  if (strcmp(new_display_data_.alarm_str, displayed_data_.alarm_str) != 0 || new_display_data_.alarm_ON != displayed_data_.alarm_ON || redraw_display_) {
    // set font
    tft.setFont(&Satisfy_Regular24pt7b);

    int16_t alarm_icon_w, alarm_icon_h;

    // change the text color to the background color
    tft.setTextColor(kDisplayBackroundColor);

    // clear old data
    if(!isThisTheFirstTime) {
      // yes! home the cursor
      tft.setCursor(alarm_row_x0_, kAlarmRowY0);

      // redraw the old value to erase
      tft.print(displayed_data_.alarm_str);

      if(displayed_data_.alarm_ON) {
        alarm_icon_w = kBellWidth;
        alarm_icon_h = kBellHeight;
      }
      else {
        alarm_icon_w = kBellFallenWidth;
        alarm_icon_h = kBellFallenHeight;
      }

      // erase bell
      tft.drawBitmap(alarm_icon_x0_, alarm_icon_y0_, (displayed_data_.alarm_ON ? kBellBitmap : kBellFallenBitmap), alarm_icon_w, alarm_icon_h, kDisplayBackroundColor);
    }

    //  Redraw new alarm data

    if(new_display_data_.alarm_ON) {
      alarm_icon_w = kBellWidth;
      alarm_icon_h = kBellHeight;
    }
    else {
      alarm_icon_w = kBellFallenWidth;
      alarm_icon_h = kBellFallenHeight;
    }

    // home the cursor
    tft.setCursor(alarm_icon_x0_, kAlarmRowY0);

    // record alarm_row_w to calculate center aligned alarm_row_x0 value
    int16_t alarm_row_y1;
    uint16_t alarm_row_w, alarm_row_h;
    // get bounds of new alarmStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(new_display_data_.alarm_str, tft.getCursorX(), tft.getCursorY(), &alarm_row_x0_, &alarm_row_y1, &alarm_row_w, &alarm_row_h);
    uint16_t graphic_width = alarm_icon_w + alarm_row_w;
    // three equal length gaps on left center and right of graphic
    uint16_t equal_gaps = (kTftWidth - graphic_width) / 3;
    alarm_row_x0_ = equal_gaps + alarm_icon_w + equal_gaps;
    alarm_icon_x0_ = equal_gaps;
    // align bell at bottom of screen
    alarm_icon_y0_ = kTftHeight - alarm_icon_h;

    // home the cursor
    tft.setCursor(alarm_row_x0_, kAlarmRowY0);

    // change the text color to foreground color
    tft.setTextColor(kDisplayAlarmColor);

    // draw the new alarmStr value
    tft.print(new_display_data_.alarm_str);

    // draw bell
    tft.drawBitmap(alarm_icon_x0_, alarm_icon_y0_, (new_display_data_.alarm_ON ? kBellBitmap : kBellFallenBitmap), alarm_icon_w, alarm_icon_h, kDisplayAlarmColor);

    // and remember the new value
    strcpy(displayed_data_.alarm_str, new_display_data_.alarm_str);
    displayed_data_.alarm_ON = new_display_data_.alarm_ON;
  }

  // // alarm cursor highlight
  // if(highlight == kMainPageSetAlarm)
  //   tft.drawRoundRect(1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, kRadiusButtonRoundRect, kDisplayColorCyan);
  // else
  //   tft.drawRoundRect(1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, kRadiusButtonRoundRect, kDisplayBackroundColor);

  redraw_display_ = false;
}

void RGBDisplay::ButtonHighlight(int16_t x, int16_t y, uint16_t w, uint16_t h, bool turnOn, int gap) {
  if(turnOn)
    tft.drawRoundRect(x - gap, y - gap, w + 2 * gap, h + 2 * gap, kRadiusButtonRoundRect, kDisplayColorCyan);
  else
    tft.drawRoundRect(x - gap, y - gap, w + 2 * gap, h + 2 * gap, kRadiusButtonRoundRect, kDisplayBackroundColor);
}

void RGBDisplay::InstantHighlightResponse(Cursor color_button) {
  if(current_page == kMainPage) {                 // MAIN PAGE
    // settings wheel cursor highlight
    ButtonHighlight(kSettingsGearX1, kSettingsGearY1, kSettingsGearWidth, kSettingsGearHeight, (highlight == kMainPageSettingsWheel), 5);

    // alarm cursor highlight
    ButtonHighlight(1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, (highlight == kMainPageSetAlarm), 0);
  }
  else if(current_page == kSettingsPage) {        // SETTINGS PAGE
    // Update WiFi Details button
    ButtonHighlight(kWiFiSettingsButtonX1, kWiFiSettingsButtonY1, kWiFiSettingsButtonW, kWiFiSettingsButtonH, (highlight == kSettingsPageWiFi), 5);

    // Update Weather and Location button
    ButtonHighlight(kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, (highlight == kSettingsPageWeather), 5);
    if(color_button == kSettingsPageWeather) DrawButton(kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, weatherStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Alarm Long Press Seconds Text
    ButtonHighlight(kAlarmLongPressSecondsX0, kAlarmLongPressSecondsY1, kAlarmLongPressSecondsW + kAlarmLongPressSecondsTriangleButtonsSize, kAlarmLongPressSecondsH, (highlight == kSettingsPageAlarmLongPressSeconds), 5);

    // Alarm Long Press Seconds Set button
    ButtonHighlight(kAlarmLongPressSecondsSetButtonX1, kAlarmLongPressSecondsSetButtonY1, kAlarmLongPressSecondsSetButtonW, kAlarmLongPressSecondsSetButtonH, (highlight == kSettingsPageSet), 5);
    DrawButton(kAlarmLongPressSecondsSetButtonX1, kAlarmLongPressSecondsSetButtonY1, kAlarmLongPressSecondsSetButtonW, kAlarmLongPressSecondsSetButtonH, setStr, kDisplayColorCyan, (color_button == kSettingsPageSet ? kDisplayColorRed : kDisplayColorOrange), kDisplayColorBlack, true);

    // Start Screensaver Button
    ButtonHighlight(kScreensaverButtonX1, kScreensaverButtonY1, kScreensaverButtonW, kScreensaverButtonH, (highlight == kSettingsPageScreensaver), 5);

    // Cancel button
    ButtonHighlight(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, (highlight == kSettingsPageCancel), 5);
  }
  else if(current_page == kWiFiSettingsPage) {          // WIFI SETTINGS PAGE
    // Set WiFi Ssid Passwd
    ButtonHighlight(kSetWiFiButtonX1, kSetWiFiButtonY1, kSetWiFiButtonW, kSetWiFiButtonH, (highlight == kWiFiSettingsPageSetSsidPasswd), 5);
    if(color_button == kWiFiSettingsPageSetSsidPasswd) DrawButton(kSetWiFiButtonX1, kSetWiFiButtonY1, kSetWiFiButtonW, kSetWiFiButtonH, setWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Connect To WiFi
    ButtonHighlight(kConnectWiFiButtonX1, kConnectWiFiButtonY1, kConnectWiFiButtonW, kConnectWiFiButtonH, (highlight == kWiFiSettingsPageConnect), 5);
    if(color_button == kWiFiSettingsPageConnect) DrawButton(kConnectWiFiButtonX1, kConnectWiFiButtonY1, kConnectWiFiButtonW, kConnectWiFiButtonH, connectWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Disconnect WiFi button
    ButtonHighlight(kDisconnectWiFiButtonX1, kDisconnectWiFiButtonY1, kDisconnectWiFiButtonW, kDisconnectWiFiButtonH, (highlight == kWiFiSettingsPageDisconnect), 5);
    if(color_button == kWiFiSettingsPageDisconnect) DrawButton(kDisconnectWiFiButtonX1, kDisconnectWiFiButtonY1, kDisconnectWiFiButtonW, kDisconnectWiFiButtonH, disconnectWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Cancel button
    ButtonHighlight(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, (highlight == kWiFiSettingsPageCancel), 5);
  }
  else if(current_page == kSoftApInputsPage) {          // SOFT AP SET WIFI SSID PASSWD PAGE
    // Save button
    ButtonHighlight(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, (highlight == kSoftApInputsPageSave), 5);
    if(color_button == kSoftApInputsPageSave) DrawButton(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, saveStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Cancel button
    ButtonHighlight(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, (highlight == kSoftApInputsPageCancel), 5);
    if(color_button == kSoftApInputsPageCancel) DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
  }
  else if(current_page == kWeatherSettingsPage) {       // WEATHER SETTINGS PAGE
    // set location button
    ButtonHighlight(kSetLocationButtonX1, kSetLocationButtonY1, kSetLocationButtonW, kSetLocationButtonH, (highlight == kWeatherSettingsPageSetLocation), 5);
    if(color_button == kWeatherSettingsPageSetLocation) DrawButton(kSetLocationButtonX1, kSetLocationButtonY1, kSetLocationButtonW, kSetLocationButtonH, setStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Toggle Units Metric/Imperial button
    ButtonHighlight(kUnitsButtonX1, kUnitsButtonY1, kUnitsButtonW, kUnitsButtonH, (highlight == kWeatherSettingsPageUnits), 5);
    if(color_button == kWeatherSettingsPageUnits) DrawButton(kUnitsButtonX1, kUnitsButtonY1, kUnitsButtonW, kUnitsButtonH, unitsStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Get WEATHER Info button
    ButtonHighlight(kFetchWeatherButtonX1, kFetchWeatherButtonY1, kFetchWeatherButtonW, kFetchWeatherButtonH, (highlight == kWeatherSettingsPageFetch), 5);
    if(color_button == kWeatherSettingsPageFetch) DrawButton(kFetchWeatherButtonX1, kFetchWeatherButtonY1, kFetchWeatherButtonW, kFetchWeatherButtonH, fetchStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Update TIME button
    ButtonHighlight(kUpdateTimeButtonX1, kUpdateTimeButtonY1, kUpdateTimeButtonW, kUpdateTimeButtonH, (highlight == kWeatherSettingsPageUpdateTime), 5);
    if(color_button == kWeatherSettingsPageUpdateTime) DrawButton(kUpdateTimeButtonX1, kUpdateTimeButtonY1, kUpdateTimeButtonW, kUpdateTimeButtonH, updateTimeStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Cancel button
    ButtonHighlight(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, (highlight == kWeatherSettingsPageCancel), 5);
  }
  else if(current_page == kLocationInputsPage) {          // LOCATION INPUTS PAGE
    // Save button
    ButtonHighlight(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, (highlight == kLocationInputsPageSave), 5);
    if(color_button == kLocationInputsPageSave) DrawButton(kSaveButtonX1, kSaveButtonY1, kSaveButtonW, kSaveButtonH, saveStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);

    // Cancel button
    ButtonHighlight(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, (highlight == kLocationInputsPageCancel), 5);
    if(color_button == kLocationInputsPageCancel) DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
  }
}

ScreenPage RGBDisplay::ClassifyUserScreenTouchInput() {
  int16_t ts_x = ts->GetTouchedPixel()->x, ts_y = ts->GetTouchedPixel()->y;
  ScreenPage returnVal = kNoPageSelected;

  // main page touch input
  if(current_page == kMainPage) {
    // if settings gear is touched
    if(ts_x >= kSettingsGearX1 && ts_x <= kSettingsGearX1 + kSettingsGearWidth && ts_y >= kSettingsGearY1 && ts_y <= kSettingsGearY1 + kSettingsGearHeight) {
      tft.drawRoundRect(kSettingsGearX1 - 1, kSettingsGearY1 - 1, kSettingsGearWidth + 2, kSettingsGearHeight + 2, kRadiusButtonRoundRect, kDisplayColorCyan);
      delay(100);
      return kSettingsPage;
    }

    // alarm area
    if(ts_y >= kAlarmRowY1) {
      tft.drawRoundRect(1, kAlarmRowY1, kTftWidth - 2, kTftHeight - kAlarmRowY1 - 1, kRadiusButtonRoundRect, kDisplayColorCyan);
      delay(100);
      return kAlarmSetPage;
    }
  }
  else if(current_page == kSettingsPage) {          // SETTINGS PAGE

    // Update WiFi Details button
    if(ts_x >= kWiFiSettingsButtonX1 && ts_x <= kWiFiSettingsButtonX1 + kWiFiSettingsButtonW && ts_y >= kWiFiSettingsButtonY1 && ts_y <= kWiFiSettingsButtonY1 + kWiFiSettingsButtonH) {
      DrawButton(kWiFiSettingsButtonX1, kWiFiSettingsButtonY1, kWiFiSettingsButtonW, kWiFiSettingsButtonH, wifiSettingsStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kWiFiSettingsPage;
    }

    // update Location Settings button
    if(ts_x >= kWeatherSettingsButtonX1 && ts_x <= kWeatherSettingsButtonX1 + kWeatherSettingsButtonW && ts_y >= kWeatherSettingsButtonY1 && ts_y <= kWeatherSettingsButtonY1 + kWeatherSettingsButtonH) {
      DrawButton(kWeatherSettingsButtonX1, kWeatherSettingsButtonY1, kWeatherSettingsButtonW, kWeatherSettingsButtonH, weatherStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kWeatherSettingsPage;
    }

    // Start Screensaver Button
    if(ts_x >= kScreensaverButtonX1 && ts_x <= kScreensaverButtonX1 + kScreensaverButtonW && ts_y >= kScreensaverButtonY1 && ts_y <= kScreensaverButtonY1 + kScreensaverButtonH) {
      DrawButton(kScreensaverButtonX1, kScreensaverButtonY1, kScreensaverButtonW, kScreensaverButtonH, screensaverStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kScreensaverPage;
    }

    // cancel button
    if(ts_x >= kCancelButtonX1 && ts_y >= kCancelButtonY1) {
      // show a little graphic of Cancel button Press
      DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kMainPage;
    }
  }
  else if(current_page == kWiFiSettingsPage) {      // WIFI SETTINGS PAGE

    // update wifi Ssid Passwd button
    if(ts_x >= kSetWiFiButtonX1 && ts_x <= kSetWiFiButtonX1 + kSetWiFiButtonW && ts_y >= kSetWiFiButtonY1 && ts_y <= kSetWiFiButtonY1 + kSetWiFiButtonH) {
      DrawButton(kSetWiFiButtonX1, kSetWiFiButtonY1, kSetWiFiButtonW, kSetWiFiButtonH, setWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kEnterWiFiPasswdPage;
    }

    // Test WiFi Connection button
    if(ts_x >= kConnectWiFiButtonX1 && ts_x <= kConnectWiFiButtonX1 + kConnectWiFiButtonW && ts_y >= kConnectWiFiButtonY1 && ts_y <= kConnectWiFiButtonY1 + kConnectWiFiButtonH) {
      DrawButton(kConnectWiFiButtonX1, kConnectWiFiButtonY1, kConnectWiFiButtonW, kConnectWiFiButtonH, connectWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      second_core_tasks_queue.push(kConnectWiFi);
      WaitForExecutionOfSecondCoreTask();
      // delay(100);
      return kWiFiSettingsPage;
    }

    // disconnect WiFi button
    if(ts_x >= kDisconnectWiFiButtonX1 && ts_x <= kDisconnectWiFiButtonX1 + kDisconnectWiFiButtonW && ts_y >= kDisconnectWiFiButtonY1 && ts_y <= kDisconnectWiFiButtonY1 + kDisconnectWiFiButtonH) {
      DrawButton(kDisconnectWiFiButtonX1, kDisconnectWiFiButtonY1, kDisconnectWiFiButtonW, kDisconnectWiFiButtonH, disconnectWiFiStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      second_core_tasks_queue.push(kDisconnectWiFi);
      WaitForExecutionOfSecondCoreTask();
      // delay(100);
      return kWiFiSettingsPage;
    }

    // cancel button
    if(ts_x >= kCancelButtonX1 && ts_y >= kCancelButtonY1) {
      // show a little graphic of Cancel button Press
      DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kSettingsPage;
    }
  }
  else if(current_page == kWeatherSettingsPage) {     // WEATHER SETTINGS PAGE

    // update kEnterWeatherLocationZipPage button
    if(ts_x >= kSetLocationButtonX1 && ts_x <= kSetLocationButtonX1 + kSetLocationButtonW && ts_y >= kSetLocationButtonY1 && ts_y <= kSetLocationButtonY1 + kSetLocationButtonH) {
      DrawButton(kSetLocationButtonX1, kSetLocationButtonY1, kSetLocationButtonW, kSetLocationButtonH, zipCodeStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kEnterWeatherLocationZipPage;
    }

    // Toggle Units Metric/Imperial button
    if(ts_x >= kUnitsButtonX1 && ts_x <= kUnitsButtonX1 + kUnitsButtonW && ts_y >= kUnitsButtonY1 && ts_y <= kUnitsButtonY1 + kUnitsButtonH) {
      wifi_stuff->weather_units_metric_not_imperial_ = !wifi_stuff->weather_units_metric_not_imperial_;
      DrawButton(kUnitsButtonX1, kUnitsButtonY1, kUnitsButtonW, kUnitsButtonH, unitsStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      // delay(100);
      wifi_stuff->SaveWeatherUnits();
      wifi_stuff->got_weather_info_ = false;
      return kWeatherSettingsPage;
    }

    // get Weather Info button
    if(ts_x >= kFetchWeatherButtonX1 && ts_x <= kFetchWeatherButtonX1 + kFetchWeatherButtonW && ts_y >= kFetchWeatherButtonY1 && ts_y <= kFetchWeatherButtonY1 + kFetchWeatherButtonH) {
      DrawButton(kFetchWeatherButtonX1, kFetchWeatherButtonY1, kFetchWeatherButtonW, kFetchWeatherButtonH, fetchStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      second_core_tasks_queue.push(kGetWeatherInfo);
      WaitForExecutionOfSecondCoreTask();
      // delay(100);
      return kWeatherSettingsPage;
    }

    // update Time button
    if(ts_x >= kUpdateTimeButtonX1 && ts_x <= kUpdateTimeButtonX1 + kUpdateTimeButtonW && ts_y >= kUpdateTimeButtonY1 && ts_y <= kUpdateTimeButtonY1 + kUpdateTimeButtonH) {
      DrawButton(kUpdateTimeButtonX1, kUpdateTimeButtonY1, kUpdateTimeButtonW, kUpdateTimeButtonH, updateTimeStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      second_core_tasks_queue.push(kUpdateTimeFromNtpServer);
      WaitForExecutionOfSecondCoreTask();
      // delay(100);
      return kMainPage;
    }

    // cancel button
    if(ts_x >= kCancelButtonX1 && ts_y >= kCancelButtonY1) {
      // show a little graphic of Cancel button Press
      DrawButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize, cancelStr, kDisplayColorCyan, kDisplayColorRed, kDisplayColorBlack, true);
      delay(100);
      return kSettingsPage;
    }
  }

  return returnVal;
}

void RGBDisplay::GoodMorningScreen() {
  tft.fillScreen(kDisplayColorBlack);
  // set font
  tft.setFont(&FreeSansBold24pt7b);

  // change the text color to the background color
  tft.setTextColor(kDisplayColorGreen);

  // yes! home the cursor
  tft.setCursor(80, 40);
  // redraw the old value to erase
  tft.print(F("GOOD"));
  // yes! home the cursor
  tft.setCursor(20, 80);
  // redraw the old value to erase
  tft.print(F("MORNING!!"));

  int16_t x0 = 80, y0 = 80;
  uint16_t edge = 160;
  
  unsigned int startTime = millis();

  // start tone
  int tone_note_index = 0;
  unsigned long next_tone_change_time = startTime;
  alarm_clock->celebrateSong(tone_note_index, next_tone_change_time);

  while(millis() - startTime < 5000)
    DrawSun(x0, y0, edge, tone_note_index, next_tone_change_time);

  tft.fillScreen(kDisplayColorBlack);
  redraw_display_ = true;
}

/* draw Sun
 * 
 * params: top left corner 'x0' and 'y0', square edge length of graphic 'edge'
 */ 
void RGBDisplay::DrawSun(int16_t x0, int16_t y0, uint16_t edge, int &tone_note_index, unsigned long &next_tone_change_time) {

  // set dimensions of sun and rays

  // sun center
  int16_t cx = x0 + edge / 2, cy = y0 + edge / 2;
  // sun radius
  int16_t sr = edge * 0.23;
  // length of rays
  int16_t rl = edge * 0.09;
  // rays inner radius
  int16_t rr = sr + edge * 0.08;
  // width of rays
  int16_t rw = 5;
  // number of rays
  uint8_t rn = 12;

  // color
  uint16_t color = kDisplayColorYellow;
  uint16_t background = kDisplayColorBlack;

  int16_t variation_prev = 0;

  // sun
  tft.fillCircle(cx, cy, sr, color);

  // eyes
  int16_t eye_offset_x = sr / 2, eye_offset_y = sr / 3, eye_r = max(sr / 8, 3);
  tft.fillCircle(cx - eye_offset_x, cy - eye_offset_y, eye_r, background);
  tft.fillCircle(cx + eye_offset_x, cy - eye_offset_y, eye_r, background);

  // draw smile
  int16_t smile_angle_deg = 37;
  int16_t smile_cy = cy - sr / 2;
  int16_t smile_r = sr * 1.1, smile_w = max(sr / 15, 3);
  for(uint8_t i = 0; i <= smile_angle_deg; i=i+2) {
    float smile_angle_rad = DEG_TO_RAD * i;
    int16_t smile_tapered_w = max(smile_w - i / 13, 1);
    // Serial.print(i); Serial.print(" "); Serial.print(smile_w); Serial.print(" "); Serial.println(smile_tapered_w);
    int16_t smile_offset_x = smile_r * sin(smile_angle_rad), smile_offset_y = smile_r * cos(smile_angle_rad);
    tft.fillCircle(cx - smile_offset_x, smile_cy + smile_offset_y, smile_tapered_w, background);
    tft.fillCircle(cx + smile_offset_x, smile_cy + smile_offset_y, smile_tapered_w, background);
  }

  // draw changing rays
  for(int16_t i = 0; i < 120; i++) {
    ResetWatchdog();
    // variation goes from 0 to 5 to 0
    int16_t i_base10_fwd = i % 10;
    int16_t i_base10_bwd = ((i / 10) + 1) * 10 - i;
    int16_t variation = min(i_base10_fwd, i_base10_bwd);
    // Serial.println(variation);
    int16_t r_variable = rr + variation;
    // draw rays
    DrawRays(cx, cy, r_variable, rl, rw, rn, i, color);
    // increase sun size
    // tft.drawCircle(cx, cy, sr + variation, color);
    DrawDenseCircle(cx, cy, sr + variation, color);
    // show for sometime
    delay(30);

    // undraw rays
    DrawRays(cx, cy, r_variable, rl, rw, rn, i, background);
    // reduce sun size
    if(variation < variation_prev){
      // tft.drawCircle(cx, cy, sr + variation_prev, background);
      DrawDenseCircle(cx, cy, sr + variation_prev + 1, background);
    }
    // delay(1000);
    variation_prev = variation;

    // celebration tone
    if(millis() > next_tone_change_time)
      alarm_clock->celebrateSong(tone_note_index, next_tone_change_time);
  }
}

/* draw rays
 * 
 * params: center cx, cy; inner radius of rays rr, length of rays rl, width of rays rw, number of rays rn, start angle degStart, color
 */ 
void RGBDisplay::DrawRays(int16_t &cx, int16_t &cy, int16_t &rr, int16_t &rl, int16_t &rw, uint8_t &rn, int16_t &degStart, uint16_t &color) {
  // rays
  for(uint8_t i = 0; i < rn; i++) {
    // find coordinates of two triangles for each ray and use fillTriangle function to draw rays
    float theta = 2 * PI * i / rn + degStart * DEG_TO_RAD;
    double rcos = rr * cos(theta), rlcos = (rr + rl) * cos(theta), rsin = rr * sin(theta), rlsin = (rr + rl) * sin(theta);
    double w2sin = rw / 2 * sin(theta), w2cos = rw / 2 * cos(theta);
    int16_t x1 = cx + rcos - w2sin;
    int16_t x2 = cx + rcos + w2sin;
    int16_t x3 = cx + rlcos + w2sin;
    int16_t x4 = cx + rlcos - w2sin;
    int16_t y1 = cy + rsin + w2cos;
    int16_t y2 = cy + rsin - w2cos;
    int16_t y3 = cy + rlsin - w2cos;
    int16_t y4 = cy + rlsin + w2cos;
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    tft.fillTriangle(x1, y1, x3, y3, x4, y4, color);
  }
}

/* drawDenseCircle
 * densely pack a circle's circumference
 */
void RGBDisplay::DrawDenseCircle(int16_t &cx, int16_t &cy, int16_t r, uint16_t &color) {
  // calculate angular resolution required
  // r*dTheta = 0.5
  double dTheta = 0.5 / static_cast<double>(r);
  // number of runs to cover quarter circle
  uint32_t n = PI / 2 / dTheta;
  // Serial.print("dTheta "); Serial.print(dTheta, 5); Serial.print(" n "); Serial.println(n);

  for(uint32_t i = 0; i < n; i++) {
    float theta = i * dTheta; // Serial.print(" theta "); Serial.println(theta);
    int16_t rcos = r * cos(theta);
    int16_t rsin = r * sin(theta);
    tft.drawPixel(cx + rcos, cy + rsin, color);
    tft.drawPixel(cx - rcos, cy + rsin, color);
    tft.drawPixel(cx + rcos, cy - rsin, color);
    tft.drawPixel(cx - rcos, cy - rsin, color);
  }
}

// make keyboard on screen
// credits: Andrew Mascolo https://github.com/AndrewMascolo/Adafruit_Stuff/blob/master/Sketches/Keyboard.ino
void RGBDisplay::MakeKeyboard(const char type[][13], char* label) {
  tft.setTextSize(2);
  // heading label
  tft.setCursor(9, 10);
  tft.setTextColor(kTextRegularColor, kDisplayBackroundColor);
  tft.print(F("Enter ")); tft.print(label); tft.print(F(":"));

  // keys
  tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);
  for (int y = 0; y < 3; y++)
  {
    int ShiftRight = 10 * pgm_read_byte(&(type[y][0]));
    for (int x = 3; x < 13; x++)
    {
      if (x >= pgm_read_byte(&(type[y][1]))) break;

      DrawKeyboardButton(8 + (23 * (x - 3)) + ShiftRight, kTextAreaHeight + (30 * y), 20,25); // this will draw the button on the screen by so many pixels
      tft.setCursor(12 + (23 * (x - 3)) + ShiftRight, kTextAreaHeight+5 + (30 * y));
      tft.print(char(pgm_read_byte(&(type[y][x]))));
    }
  }
  //ShiftKey
  DrawKeyboardButton(5, kTextAreaHeight + 60, 30, 25);
  tft.setTextColor(kTextHighLightColor, kKeyboardButtonFillColor);
  tft.setCursor(15, kTextAreaHeight + 65);
  tft.print('^');

  //Special Characters
  DrawKeyboardButton(5, kTextAreaHeight + 90, 30, 25);
  tft.setCursor(9, kTextAreaHeight + 95);
  tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);
  tft.print(F("SP"));

  //BackSpace
  DrawKeyboardButton(200, kTextAreaHeight + 60, 30, 25);
  tft.setCursor(204, kTextAreaHeight + 65);
  tft.print(F("BS"));

  //Return
  DrawKeyboardButton(200, kTextAreaHeight + 90, 30, 25);
  tft.setCursor(204, kTextAreaHeight + 95);
  tft.print(F("RT"));

  //Spacebar
  DrawKeyboardButton(47, kTextAreaHeight + 90, 140, 25);
  tft.setCursor(65, kTextAreaHeight + 95);
  tft.print(F("SPACE BAR"));

  // Cancel button
  DrawKeyboardButton(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize);
  tft.setCursor(kCancelButtonX1 + 15, kCancelButtonY1 + 10);
  tft.print(F("X"));
}

// helper function for MakeKeyboard
// credits: Andrew Mascolo https://github.com/AndrewMascolo/Adafruit_Stuff/blob/master/Sketches/Keyboard.ino
void RGBDisplay::DrawKeyboardButton(int x, int y, int w, int h) {
  // grey
  tft.fillRoundRect(x - 3, y + 3, w, h, 3, 0x8888); //Button Shading

  // white
  tft.fillRoundRect(x, y, w, h, 3, 0xffff);// outter button color

  //red
  tft.fillRoundRect(x + 1, y + 1, w - 1 * 2, h - 1 * 2, 3, kKeyboardButtonFillColor); //inner button color
}

byte RGBDisplay::IsTouchWithin(int x, int y, int w, int h) {
  // tft.fillCircle(X, Y, 2, 0x0FF0);
  return ((((ts->GetTouchedPixel())->x>=x)&&((ts->GetTouchedPixel())->x<=x + w)) & (((ts->GetTouchedPixel())->y>=y)&&((ts->GetTouchedPixel())->y<=y + h)));
}

// get keyboard presses on keyboard made by MakeKeyboard
// credits: Andrew Mascolo https://github.com/AndrewMascolo/Adafruit_Stuff/blob/master/Sketches/Keyboard.ino
bool RGBDisplay::GetKeyboardPress(char * textBuffer, char* label, char * textReturn) {
  char key = 0;
  static bool shift = true, lastSh = true, special = false, lastSp = false;
  static char bufIndex = 0;

  if (ts->IsTouched())
  {
    // check if cancel button is pressed
    if (IsTouchWithin(kCancelButtonX1, kCancelButtonY1, kCancelButtonSize, kCancelButtonSize))
    {
      return false;
    }

    //ShiftKey
    if (IsTouchWithin(5, kTextAreaHeight + 60, 30, 25))
    {
      shift = !shift;
      delay(200);
    }

    //Special Characters
    if (IsTouchWithin(5, kTextAreaHeight + 90, 30, 25))
    {
      special = !special;
      delay(200);
    }

    // re-draw keyboard if...
    if (special != lastSp || shift != lastSh)
    {
      if (special)
      {
        if (shift)
        {
          tft.fillScreen(kDisplayBackroundColor);
          MakeKeyboard(Mobile_SymKeys, label);
        }
        else
        {
          tft.fillScreen(kDisplayBackroundColor);
          MakeKeyboard(Mobile_NumKeys, label);
        }
      }
      else
      {
        if (shift)
        {
          tft.fillScreen(kDisplayBackroundColor);
          MakeKeyboard(Mobile_KB_Capitals, label);
          tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);
        }
        else
        {
          tft.fillScreen(kDisplayBackroundColor);
          MakeKeyboard(Mobile_KB_Smalls, label);
          tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);
        }
      }

      if (special)
        tft.setTextColor(kTextHighLightColor, kKeyboardButtonFillColor);
      else
        tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);

      tft.setCursor(9, kTextAreaHeight + 95);
      tft.print(F("SP"));

      if (shift)
        tft.setTextColor(kTextHighLightColor, kKeyboardButtonFillColor);
      else
        tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);

      tft.setCursor(15, kTextAreaHeight + 65);
      tft.print('^');

      lastSp = special;
      lastSh = shift;
    }

    for (int y = 0; y < 3; y++)
    {
      int ShiftRight;
      if (special)
      {
        if (shift)
          ShiftRight = 10 * pgm_read_byte(&(Mobile_SymKeys[y][0]));
        else
          ShiftRight = 10 * pgm_read_byte(&(Mobile_NumKeys[y][0]));
      }
      else
      {
        if (shift)
          ShiftRight = 10 * pgm_read_byte(&(Mobile_KB_Capitals[y][0]));
        else
          ShiftRight = 10 * pgm_read_byte(&(Mobile_KB_Smalls[y][0]));
      }

      for (int x = 3; x < 13; x++)
      {
        if (x >=  (special ? (shift ? pgm_read_byte(&(Mobile_SymKeys[y][1])) : pgm_read_byte(&(Mobile_NumKeys[y][1]))) : pgm_read_byte(&(Mobile_KB_Capitals[y][1])) )) break;

        if (IsTouchWithin(8 + (23 * (x - 3)) + ShiftRight, kTextAreaHeight + (30 * y), 20,25)) // this will draw the button on the screen by so many pixels
        {
          if (bufIndex < (kWifiSsidPasswordLengthMax))
          {
            delay(200);

            if (special)
            {
              if (shift)
                textBuffer[bufIndex] = pgm_read_byte(&(Mobile_SymKeys[y][x]));
              else
                textBuffer[bufIndex] = pgm_read_byte(&(Mobile_NumKeys[y][x]));
            }
            else
              textBuffer[bufIndex] = (pgm_read_byte(&(Mobile_KB_Capitals[y][x])) + (shift ? 0 : ('a' - 'A')));

            bufIndex++;
          }
          break;
        }
      }
    }

    //Spacebar
    if (IsTouchWithin(47, kTextAreaHeight + 90, 140, 25))
    {
      textBuffer[bufIndex++] = ' ';
      delay(200);
    }

    //BackSpace
    if (IsTouchWithin(200, kTextAreaHeight + 60, 30, 25))
    {
      if ((bufIndex) > 0)
        bufIndex--;
      textBuffer[bufIndex] = 0;
      // clear writing pad
      tft.fillRect(15, kTextAreaHeight - 30, tft.width() - 40, 20, kDisplayBackroundColor);
      delay(200);
    }

    //Return
    if (IsTouchWithin(200, kTextAreaHeight + 90, 30, 25))
    {
      // Serial.println(textBuffer);
      strcpy(textReturn, textBuffer);
      while (bufIndex > 0)
      {
        bufIndex--;
        textBuffer[bufIndex] = 0;
      }
      // clear writing pad
      tft.fillRect(15, kTextAreaHeight - 30, tft.width() - 40, 20, kDisplayBackroundColor);
    }
  }
  tft.setTextColor(kTextRegularColor, kKeyboardButtonFillColor);
  tft.setCursor(15, kTextAreaHeight - 30);
  tft.print(textBuffer);
  // Serial.println(textBuffer);
  return true;
}

// get user text input from on-screen keyboard
bool RGBDisplay::GetUserOnScreenTextInput(char* label, char* return_text) {
  bool ret = false;

  tft.fillScreen(kDisplayBackroundColor);
  tft.setFont(NULL);

  MakeKeyboard(Mobile_KB_Capitals, label);

  // buffer for user input
  char user_input_buffer[kWifiSsidPasswordLengthMax + 1] = "";

  // get user input
  while(1) {
    ResetWatchdog();
    // See if there's any  touch data for us
    ret = GetKeyboardPress(user_input_buffer, label, return_text);
    if(!ret)
      break;

    //print the text
    tft.setCursor(10,30);
    tft.println(return_text);
    if(strcmp(return_text, "") != 0) {
      PrintLn(return_text);
      delay(1000);
      break;
    }
  }
  tft.setTextSize(1);
  return ret;
}

// void RGBDisplay::fastDrawBitmap(int16_t x, int16_t y, uint8_t* bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
//   toggler = false;
//   elapsedMillis timer1;
//   if(toggler) {
//     SPI.beginTransaction( SPISettings(150000000, MSBFIRST, SPI_MODE0) );
//     digitalWrite(TFT_CS, 0);  // indicate "transfer"
//     digitalWrite(TFT_DC, 0);  // indicate "command"
//     SPI.transfer(0x2A);              // send column span command
//     digitalWrite(TFT_DC, 1);  // indicate "data"
//     SPI.transfer16(x);//     >> 8;      // send Xmin
//     SPI.transfer16(x+w-1);// >> 8;      // send Xmax
//     digitalWrite(TFT_DC, 0);  // indicate "command"
//     SPI.transfer(0x2B);              // send row span command
//     digitalWrite(TFT_DC, 1);  // indicate "data"
//     SPI.transfer16(y);//     >> 8;      // send Ymin
//     SPI.transfer16(y+h-1);// >> 8;      // send Ymax
//     digitalWrite(TFT_DC, 0);  // indicate "command"
//     SPI.transfer(0x2C);              // send write command
//     digitalWrite(TFT_DC, 1);  // indicate "data"
//     int16_t byteWidth = (w + 7) >> 3;          // bitmap width in bytes
//     int8_t bits8 = 0;
//     for (int16_t j = 0; j < h; j++) {
//       for (int16_t i = 0; i < w; i++)
//       {
//         bits8 = i & 7 ? bits8 << 1 : bitmap[j * byteWidth + (i >> 3)];  // fetch next pixel
//         uint16_t c = bits8 < 0 ? color : bg;   // select color
//         tft.SPI_WRITE16(c);
//       }
//     }
//     // pinMode(TFT_CS, INPUT); // Set CS_Pin to high impedance to allow pull-up to reset CS to inactive.
//     // digitalWrite(TFT_CS, 1); // Enable internal pull-up
//     digitalWrite(TFT_CS, 1);                   // indicate "idle"
//     SPI.endTransaction();
//   }
//   else {
//     Serial.println("***SEE_HERE***");
//     // make a 16 bit rgbBitmap buffer to test send time of drawRGBBitmap
//     // int bitmapSize = w*h/8 + (w*h%8 > 0 ? 1 : 0);
//     // for (int r = 0; r<bitmapSize; r++) {
//     //   Serial.print(bitmap[r]);Serial.print(charSpace);
//     // }
//     // Serial.println();
//     int bufferSize = w;// * h;
//     uint16_t buffer16Bit[bufferSize];
//     tft.startWrite();
//     tft.setAddrWindow(x, y, w, h);
    
//     int16_t byteWidth = (w + 7) >> 3;          // bitmap width in bytes
//     int8_t bits8 = 0;
//     for (int16_t j = 0; j < h; j++) {
//       int bufi = 0;
//       for (int16_t i = 0; i < w; i++)
//       {
//         bits8 = i & 7 ? bits8 << 1 : bitmap[j * byteWidth + (i >> 3)];  // fetch next pixel
//         uint16_t c = bits8 < 0 ? color : bg;   // select color
//         buffer16Bit[bufi] = c;
//         bufi++;
//         // tft.writePixels(&c, 1);
//       }
//       tft.writePixels(buffer16Bit, bufferSize);
//     }
    
//     tft.endWrite();
//     // alarmClock->serialTimeStampPrefix(); Serial.print(charSpace); Serial.print(timer1);
//     // tft.drawRGBBitmap(x, y, buffer16Bit, w, h); // Copy to screen
//     alarmClock->serialTimeStampPrefix();
//     Serial.println("***DONE***");
//   }
  
//   Serial.print(" w "); Serial.print(w);Serial.print(" h "); Serial.print(h); Serial.print(" fastDrawBitmapTime "); Serial.print(toggler); Serial.print(charSpace); Serial.println(timer1);

//   toggler = !toggler;
// }
