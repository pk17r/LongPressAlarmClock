#include "pin_defs.h"
#include "rgb_display_class.h"
#include "alarm_clock_main.h"
#include <Arduino.h>

void rgb_display_class::setAlarmScreen(bool firstDraw, int16_t ts_x, int16_t ts_y) {

  int16_t gap_x = tft.width() / 11;
  int16_t gap_y = tft.height() / 9;
  int16_t hr_x = gap_x / 2, min_x = 2.25*gap_x, amPm_x = 4.25*gap_x, onOff_x = 6.5*gap_x, setCancel_x = 9*gap_x;
  int16_t time_y = 6*gap_y, onSet_y = 3.5*gap_y, offCancel_y = 6.5*gap_y, inc_y = 4*gap_y, dec_y = 8*gap_y;
  int16_t incB_y = time_y - 3*gap_y, decB_y = time_y + gap_y;
  uint16_t onFill = Display_Color_Green, offFill = Display_Color_Black, borderColor = Display_Color_Cyan;
  uint16_t button_w = 2*gap_x, button_h = 2*gap_y;

  if(firstDraw) {
    // make alarm set page

    tft.fillScreen(Display_Backround_Color);

    // set title font
    tft.setFont(&Satisfy_Regular18pt7b);

    char* title = "Set Alarm";

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    int16_t title_x0, title_y0;
    uint16_t title_w, title_h;
    // get bounds of title on tft display (with background color as this causes a blink)
    tft.getTextBounds(title, 0, 0, &title_x0, &title_y0, &title_w, &title_h);

    int16_t title_x = (tft.width() - title_w) / 2;
    int16_t title_y = 2*gap_y;

    // change the text color to the Alarm color
    tft.setTextColor(Display_Alarm_Color);
    tft.setCursor(title_x, title_y);
    tft.print(title);

    // font color inside
    tft.setTextColor(Display_Color_Green);

    // print alarm time
    tft.setCursor(hr_x, time_y);
    tft.print(main->alarmHrSetPage);
    tft.setCursor(min_x, time_y);
    if(main->alarmMinSetPage < 10)
      tft.print('0');
    tft.print(main->alarmMinSetPage);
    tft.setCursor(amPm_x, time_y);
    if(main->alarmIsAmSetPage)
      tft.print(amLabel);
    else
      tft.print(pmLabel);

    // draw increase / dec buttons
    // hour
    drawTriangleButton(hr_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    drawTriangleButton(hr_x, decB_y, gap_x, gap_y, false, borderColor, offFill);
    // min
    drawTriangleButton(min_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    drawTriangleButton(min_x, decB_y, gap_x, gap_y, false, borderColor, offFill);
    // am / pm
    drawTriangleButton(amPm_x, incB_y, gap_x, gap_y, true, borderColor, offFill);
    drawTriangleButton(amPm_x, decB_y, gap_x, gap_y, false, borderColor, offFill);

    // ON button
    drawButton(onOff_x, onSet_y, button_w, button_h, "ON", borderColor, onFill, offFill, main->alarmOnSetPage);
    // OFF button
    drawButton(onOff_x, offCancel_y, button_w, button_h, "OFF", borderColor, onFill, offFill, !main->alarmOnSetPage);
    // Set button
    drawButton(setCancel_x, onSet_y, button_w, button_h, "Set", borderColor, Display_Color_Orange, offFill, true);
    // Cancel button
    drawButton(setCancel_x, offCancel_y, button_w, button_h, "X", borderColor, Display_Color_Orange, offFill, true);

  }
  else {
    // user action

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
      drawTriangleButton(triangle_x, triangle_y, gap_x, gap_y, isUp, borderColor, onFill);
      // clear old values
      tft.setFont(&Satisfy_Regular18pt7b);
      tft.setCursor(triangle_x, time_y);
      tft.setTextColor(Display_Backround_Color);
      if(userButtonClick <= 2)
        tft.print(main->alarmHrSetPage);
      else if(userButtonClick <= 4) {
        if(main->alarmMinSetPage < 10) tft.print('0');
        tft.print(main->alarmMinSetPage);
      }
      else {
        if(main->alarmIsAmSetPage)
          tft.print(amLabel);
        else
          tft.print(pmLabel);
      }
      // update value
      if(userButtonClick <= 2) {
        if(isUp) {  // increase hour
          if(main->alarmHrSetPage < 12)
            main->alarmHrSetPage++;
          else
            main->alarmHrSetPage = 1;
        }
        else {  // decrease hour
          if(main->alarmHrSetPage > 1)
            main->alarmHrSetPage--;
          else
            main->alarmHrSetPage = 12;
        }
      }
      else if(userButtonClick <= 4) {
        if(isUp) {  // increase min
          if(main->alarmMinSetPage  < 59)
            main->alarmMinSetPage++;
          else
            main->alarmMinSetPage = 0;
        }
        else {  // decrease min
          if(main->alarmMinSetPage  > 0)
            main->alarmMinSetPage--;
          else
            main->alarmMinSetPage = 59;
        }
      }
      else  // turn alarm On or Off
        main->alarmIsAmSetPage = !main->alarmIsAmSetPage;
      // print updated value
      tft.setCursor(triangle_x, time_y);
      tft.setTextColor(Display_Color_Green);
      if(userButtonClick <= 2)
        tft.print(main->alarmHrSetPage);
      else if(userButtonClick <= 4) {
        if(main->alarmMinSetPage < 10) tft.print('0');
        tft.print(main->alarmMinSetPage);
      }
      else {
        if(main->alarmIsAmSetPage)
          tft.print(amLabel);
        else
          tft.print(pmLabel);
      }
      // wait a little
      unsigned long waitTime = 200;
      if(userButtonClick == 3 || userButtonClick == 4)  // wait less for minutes
        waitTime = 100;
      delay(waitTime);
      // turn triangle Off
      drawTriangleButton(triangle_x, triangle_y, gap_x, gap_y, isUp, borderColor, offFill);
    }
    else if(userButtonClick == 7 || userButtonClick == 8) {
      // On or Off button pressed
      if((userButtonClick == 7 && !main->alarmOnSetPage) || (userButtonClick == 8 && main->alarmOnSetPage)) {
        // toggle alarm
        main->alarmOnSetPage = !main->alarmOnSetPage;
        // draw new ON button with push effect
        drawButton(onOff_x, onSet_y, button_w, button_h, "ON", borderColor, Display_Alarm_Color, offFill, main->alarmOnSetPage);
        // draw new OFF button
        drawButton(onOff_x, offCancel_y, button_w, button_h, "OFF", borderColor, onFill, offFill, !main->alarmOnSetPage);
        delay(100);
        // draw new ON button
        drawButton(onOff_x, onSet_y, button_w, button_h, "ON", borderColor, onFill, offFill, main->alarmOnSetPage);
      }
      else if(userButtonClick == 7 && main->alarmOnSetPage) {
        // alarm is On but user pressed On button
        // show a little graphic of input taken
        drawButton(onOff_x, onSet_y, button_w, button_h, "ON", borderColor, Display_Alarm_Color, offFill, main->alarmOnSetPage);
        delay(100);
        drawButton(onOff_x, onSet_y, button_w, button_h, "ON", borderColor, onFill, offFill, main->alarmOnSetPage);
      }
      else if(userButtonClick == 8 && !main->alarmOnSetPage) {
        // alarm is Off but user pressed Off button
        // show a little graphic of input taken
        drawButton(onOff_x, offCancel_y, button_w, button_h, "OFF", borderColor, Display_Alarm_Color, offFill, !main->alarmOnSetPage);
        delay(100);
        drawButton(onOff_x, offCancel_y, button_w, button_h, "OFF", borderColor, onFill, offFill, !main->alarmOnSetPage);
      }
    }
    else if(userButtonClick == 9 || userButtonClick == 10) {
      // set or cancel button pressed
      if(userButtonClick == 9) {  // set button pressed
        // show a little graphic of Set Button Press
        drawButton(setCancel_x, onSet_y, button_w, button_h, "Set", borderColor, Display_Color_Red, offFill, true);
        // set alarm page values to system
        main->alarmHr = main->alarmHrSetPage;
        main->alarmMin = main->alarmMinSetPage;
        main->alarmIsAm = main->alarmIsAmSetPage;
        main->alarmOn = main->alarmOnSetPage;
      }
      else  // show a little graphic of Cancel button Press
        drawButton(setCancel_x, offCancel_y, button_w, button_h, "X", borderColor, Display_Color_Red, offFill, true);
      // wait a little
      delay(100);
      // go back to main page
      main->setPage(main->mainPage);
    }
    
  }
}

void rgb_display_class::drawButton(int16_t x, int16_t y, uint16_t w, uint16_t h, char* label, uint16_t borderColor, uint16_t onFill, uint16_t offFill, bool isOn) {
  int16_t r = 3;
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor((isOn ? onFill : offFill));
  int16_t title_x0, title_y0;
  uint16_t title_w, title_h;
  // get bounds of title on tft display (with background color as this causes a blink)
  tft.getTextBounds(label, x, y + h, &title_x0, &title_y0, &title_w, &title_h);
  // make button
  tft.fillRoundRect(x, y, w, h, r, (isOn ? onFill : offFill));
  tft.drawRoundRect(x, y, w, h, r, borderColor);
  tft.setTextColor((isOn ? offFill : onFill));
  title_x0 = x + (w - title_w) / 2;
  title_y0 = y + h / 2 + title_h / 2;
  tft.setCursor(title_x0, title_y0);
  tft.print(label);
}

void rgb_display_class::drawTriangleButton(int16_t x, int16_t y, uint16_t w, uint16_t h, bool isUp, uint16_t borderColor, uint16_t fillColor) {
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

void rgb_display_class::screensaver() {
  // In global declarations:
  GFXcanvas16 canvas(tft.width(), tft.height()); // 128x32 pixel canvas
  canvas.fillScreen(Display_Backround_Color);
  canvas.setTextWrap(false);
  canvas.setFont(&ComingSoon_Regular70pt7b);
  canvas.getTextBounds(newDisplayData.timeHHMM, 0, 0, &gap_right_x, &gap_up_y, &tft_HHMM_w, &tft_HHMM_h);
  // Serial.print("gap_right_x "); Serial.print(gap_right_x); Serial.print(" gap_up_y "); Serial.print(gap_up_y); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h);
  // move around
  const int16_t adder = 1;
  tft_HHMM_x0 += (screensaverMoveRight ? adder : -adder);
  tft_HHMM_y0 += (screensaverMoveDown ? adder : -adder);
  // set direction
  if(tft_HHMM_x0 <= 0) {
    screensaverMoveRight = true;
    pickNewRandomColor();
  }
  else if(tft_HHMM_x0 + gap_right_x + tft_HHMM_w >= tft.width()) {
    screensaverMoveRight = false;
    pickNewRandomColor();
  }
  if(tft_HHMM_y0 + gap_up_y <= 0)  {
    screensaverMoveDown = true;
    pickNewRandomColor();
  }
  else if(tft_HHMM_y0 + gap_up_y + tft_HHMM_h >= tft.height())  {
    screensaverMoveDown = false;
    pickNewRandomColor();
  }
  // Serial.print("x0 "); Serial.print(tft_HHMM_x0); Serial.print(" y0 "); Serial.println(tft_HHMM_y0);
  // Serial.print("screensaverMoveRight "); Serial.print(screensaverMoveRight); Serial.print(" screensaverMoveDown "); Serial.println(screensaverMoveDown);
  // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
  canvas.setCursor(tft_HHMM_x0, tft_HHMM_y0);
  // canvas.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
  canvas.setTextColor(colorPickerWheelBright[currentRandomColorIndex]);
  canvas.print(newDisplayData.timeHHMM);
  // In code later:
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), tft.width(), tft.height()); // Copy to screen
}

void rgb_display_class::pickNewRandomColor() {
  int newIndex = currentRandomColorIndex;
  while(newIndex == currentRandomColorIndex)
    newIndex = random(0, COLOR_PICKER_WHEEL_SIZE - 1);
  currentRandomColorIndex = newIndex;
  // Serial.println(currentRandomColorIndex);
}

void rgb_display_class::displayTimeUpdate() {
  bool isThisTheFirstTime = strcmp(displayedData.timeSS, "") == 0;

  // HH:MM string and AM/PM string
  if (strcmp(newDisplayData.timeHHMM, displayedData.timeHHMM) != 0 || redrawDisplay) {

    // HH:MM

    // set font
    // tft.setTextSize(1);
    tft.setFont(&FreeSansBold48pt7b);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // clear old time if it was there
    if(!isThisTheFirstTime) {
      // home the cursor to currently displayed text location
      tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);

      // redraw the old value to erase
      tft.print(displayedData.timeHHMM);
      // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
    }

    // get bounds of new HH:MM string on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.timeHHMM, 0, 0, &gap_right_x, &gap_up_y, &tft_HHMM_w, &tft_HHMM_h);
    // Serial.print("gap_right_x "); Serial.print(gap_right_x); Serial.print(" gap_up_y "); Serial.print(gap_up_y); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 

    // home the cursor
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
    // Serial.print("X0 "); Serial.print(TIME_ROW_X0); Serial.print(" Y0 "); Serial.print(TIME_ROW_Y0); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 
    // tft.drawRect(TIME_ROW_X0 + gap_right_x, TIME_ROW_Y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);

    // change the text color to foreground color
    tft.setTextColor(Display_Time_Color);

    // draw the new time value
    tft.print(newDisplayData.timeHHMM);
    // tft.setTextSize(1);
    // delay(2000);

    // and remember the new value
    strcpy(displayedData.timeHHMM, newDisplayData.timeHHMM);

    // AM/PM

    // set font
    tft.setFont(&FreeSans18pt7b);

    // clear old AM/PM
    if(!isThisTheFirstTime && displayedData._12hourMode) {
      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);

      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // redraw the old value to erase
      if(displayedData._pmNotAm)
        tft.print(pmLabel);
      else
        tft.print(amLabel);
    }

    // draw new AM/PM
    if(newDisplayData._12hourMode) {
      // set test location of Am/Pm
      tft_AmPm_x0 = TIME_ROW_X0 + gap_right_x + tft_HHMM_w + 2 * DISPLAY_TEXT_GAP;
      tft_AmPm_y0 = TIME_ROW_Y0 + gap_up_y / 2;

      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);
      // Serial.print("tft_AmPm_x0 "); Serial.print(tft_AmPm_x0); Serial.print(" y0 "); Serial.print(tft_AmPm_y0); Serial.print(" tft.getCursorX() "); Serial.print(tft.getCursorX()); Serial.print(" tft.getCursorY() "); Serial.println(tft.getCursorY()); 

      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // get bounds of new AM/PM string on tft display (with background color as this causes a blink)
      int16_t tft_AmPm_x1, tft_AmPm_y1;
      uint16_t tft_AmPm_w, tft_AmPm_h;
      tft.getTextBounds((newDisplayData._pmNotAm ? pmLabel : amLabel), tft.getCursorX(), tft.getCursorY(), &tft_AmPm_x1, &tft_AmPm_y1, &tft_AmPm_w, &tft_AmPm_h);
      // Serial.print("AmPm_x1 "); Serial.print(tft_AmPm_x1); Serial.print(" y1 "); Serial.print(tft_AmPm_y1); Serial.print(" w "); Serial.print(tft_AmPm_w); Serial.print(" h "); Serial.println(tft_AmPm_h); 

      // calculate tft_AmPm_y0 to align top with HH:MM
      tft_AmPm_y0 -= tft_AmPm_y1 - TIME_ROW_Y0 - gap_up_y;
      // Serial.print("tft_AmPm_y0 "); Serial.println(tft_AmPm_y0);

      // home the cursor
      tft.setCursor(tft_AmPm_x0, tft_AmPm_y0);

      // change the text color to foreground color
      tft.setTextColor(Display_Time_Color);

      // draw the new time value
      if(newDisplayData._pmNotAm)
        tft.print(pmLabel);
      else
        tft.print(amLabel);
    }

    // and remember the new value
    displayedData._12hourMode = newDisplayData._12hourMode;
    displayedData._pmNotAm = newDisplayData._pmNotAm;
  }

  // :SS string
  if (strcmp(newDisplayData.timeSS, displayedData.timeSS) != 0 || redrawDisplay) {
    // set font
    tft.setFont(&FreeSans24pt7b);

    // clear old seconds
    if(!isThisTheFirstTime) {
      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // home the cursor
      tft.setCursor(tft_SS_x0, TIME_ROW_Y0);

      // change the text color to the background color
      tft.setTextColor(Display_Backround_Color);

      // redraw the old value to erase
      tft.print(displayedData.timeSS);
    }

    // fill new home values
    tft_SS_x0 = TIME_ROW_X0 + gap_right_x + tft_HHMM_w + DISPLAY_TEXT_GAP;

    // home the cursor
    tft.setCursor(tft_SS_x0, TIME_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Time_Color);

    // draw the new time value
    tft.print(newDisplayData.timeSS);

    // and remember the new value
    strcpy(displayedData.timeSS, newDisplayData.timeSS);
  }
  
  // date string center aligned
  if (strcmp(newDisplayData.dateStr, displayedData.dateStr) != 0 || redrawDisplay) {
    // set font
    tft.setFont(&Satisfy_Regular24pt7b);

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // clear old data
    if(!isThisTheFirstTime) {
      // yes! home the cursor
      tft.setCursor(date_row_x0, DATE_ROW_Y0);

      // redraw the old value to erase
      tft.print(displayedData.dateStr);
    }

    // home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // record date_row_w to calculate center aligned date_row_x0 value
    int16_t date_row_y1;
    uint16_t date_row_w, date_row_h;
    // get bounds of new dateStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.dateStr, tft.getCursorX(), tft.getCursorY(), &date_row_x0, &date_row_y1, &date_row_w, &date_row_h);
    date_row_x0 = (tft.width() - date_row_w) / 2;

    // home the cursor
    tft.setCursor(date_row_x0, DATE_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Date_Color);

    // draw the new dateStr value
    tft.print(newDisplayData.dateStr);

    // and remember the new value
    strcpy(displayedData.dateStr, newDisplayData.dateStr);
  }

  // alarm string center aligned
  if (strcmp(newDisplayData.alarmStr, displayedData.alarmStr) != 0 || newDisplayData._alarmOn != displayedData._alarmOn || redrawDisplay) {
    // set font
    tft.setFont(&Satisfy_Regular24pt7b);

    int16_t alarm_icon_w, alarm_icon_h;

    // change the text color to the background color
    tft.setTextColor(Display_Backround_Color);

    // clear old data
    if(!isThisTheFirstTime) {
      // yes! home the cursor
      tft.setCursor(alarm_row_x0, ALARM_ROW_Y0);

      // redraw the old value to erase
      tft.print(displayedData.alarmStr);

      if(displayedData._alarmOn) {
        alarm_icon_w = bell_w;
        alarm_icon_h = bell_h;
      }
      else {
        alarm_icon_w = bell_fallen_w;
        alarm_icon_h = bell_fallen_h;
      }

      // erase bell
      tft.drawBitmap(alarm_icon_x0, alarm_icon_y0, (displayedData._alarmOn ? bell_bitmap : bell_fallen_bitmap), alarm_icon_w, alarm_icon_h, Display_Backround_Color);
    }

    //  Redraw new alarm data

    if(newDisplayData._alarmOn) {
      alarm_icon_w = bell_w;
      alarm_icon_h = bell_h;
    }
    else {
      alarm_icon_w = bell_fallen_w;
      alarm_icon_h = bell_fallen_h;
    }

    // home the cursor
    tft.setCursor(alarm_icon_x0, ALARM_ROW_Y0);

    // record alarm_row_w to calculate center aligned alarm_row_x0 value
    int16_t alarm_row_y1;
    uint16_t alarm_row_w, alarm_row_h;
    // get bounds of new alarmStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.alarmStr, tft.getCursorX(), tft.getCursorY(), &alarm_row_x0, &alarm_row_y1, &alarm_row_w, &alarm_row_h);
    uint16_t graphic_width = alarm_icon_w + alarm_row_w;
    // three equal length gaps on left center and right of graphic
    uint16_t equal_gaps = (tft.width() - graphic_width) / 3;
    alarm_row_x0 = equal_gaps + alarm_icon_w + equal_gaps;
    alarm_icon_x0 = equal_gaps;
    // align bell at bottom of screen
    alarm_icon_y0 = tft.height() - alarm_icon_h;

    // home the cursor
    tft.setCursor(alarm_row_x0, ALARM_ROW_Y0);

    // change the text color to foreground color
    tft.setTextColor(Display_Alarm_Color);

    // draw the new alarmStr value
    tft.print(newDisplayData.alarmStr);

    // draw bell
    tft.drawBitmap(alarm_icon_x0, alarm_icon_y0, (newDisplayData._alarmOn ? bell_bitmap : bell_fallen_bitmap), alarm_icon_w, alarm_icon_h, Display_Alarm_Color);

    // and remember the new value
    strcpy(displayedData.alarmStr, newDisplayData.alarmStr);
    displayedData._alarmOn = newDisplayData._alarmOn;
  }

  redrawDisplay = false;
}

void rgb_display_class::goodMorningScreen() {
  tft.fillScreen(Display_Color_Black);
  // set font
  tft.setFont(&FreeSansBold24pt7b);

  // change the text color to the background color
  tft.setTextColor(Display_Color_Green);

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
  while(millis() - startTime < 5000)
    drawSun(x0, y0, edge);

  tft.fillScreen(Display_Color_Black);
  main->refreshRtcTime = true;
  redrawDisplay = true;
}

/* draw Sun
 * 
 * params: top left corner 'x0' and 'y0', square edge length of graphic 'edge'
 */ 
void rgb_display_class::drawSun(int16_t x0, int16_t y0, uint16_t edge) {

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
  uint16_t color = Display_Color_Yellow;
  uint16_t background = Display_Color_Black;

  int16_t variation_prev = 0;

  // sun
  tft.fillCircle(cx, cy, sr, color);

  // eyes
  int16_t eye_offset_x = sr / 2, eye_offset_y = sr / 3, eye_r = max(sr / 8, 3);
  tft.fillCircle(cx - eye_offset_x, cy - eye_offset_y, eye_r, background);
  tft.fillCircle(cx + eye_offset_x, cy - eye_offset_y, eye_r, background);

  // smile
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

  // rays
  for(int16_t i = 0; i < 120; i++) {
    // variation goes from 0 to 5 to 0
    int16_t i_base10_fwd = i % 10;
    int16_t i_base10_bwd = ((i / 10) + 1) * 10 - i;
    int16_t variation = min(i_base10_fwd, i_base10_bwd);
    // Serial.println(variation);
    int16_t r_variable = rr + variation;
    // draw rays
    drawRays(cx, cy, r_variable, rl, rw, rn, i, color);
    // increase sun size
    // tft.drawCircle(cx, cy, sr + variation, color);
    drawDenseCircle(cx, cy, sr + variation, color);
    // show for sometime
    delay(30);

    // undraw rays
    drawRays(cx, cy, r_variable, rl, rw, rn, i, background);
    // reduce sun size
    if(variation < variation_prev){
      // tft.drawCircle(cx, cy, sr + variation_prev, background);
      drawDenseCircle(cx, cy, sr + variation_prev + 1, background);
    }
    // delay(1000);
    variation_prev = variation;
  }
}

/* draw rays
 * 
 * params: center cx, cy; inner radius of rays rr, length of rays rl, width of rays rw, number of rays rn, start angle degStart, color
 */ 
void rgb_display_class::drawRays(int16_t &cx, int16_t &cy, int16_t &rr, int16_t &rl, int16_t &rw, uint8_t &rn, int16_t &degStart, uint16_t &color) {
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
void rgb_display_class::drawDenseCircle(int16_t &cx, int16_t &cy, int16_t r, uint16_t &color) {
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
