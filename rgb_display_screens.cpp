#include "pin_defs.h"
#include "rgb_display_class.h"
#include <Arduino.h>


void rgb_display_class::displayHHMM() {
  bool isThisTheFirstTime = strcmp(displayedData.timeHHMM, "") == 0;

  // HH:MM

  // set font
  // tft.setTextSize(1);
  if(screensaver)
    tft.setFont(&ComingSoon_Regular70pt7b);
  else
    tft.setFont(&FreeSansBold48pt7b);

  // change the text color to the background color
  tft.setTextColor(Display_Backround_Color);

  // clear old time if it was there
  if(!isThisTheFirstTime) {
    // home the cursor to currently displayed text location
    if(!screensaver)
      tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
    else
      tft.setCursor(tft_HHMM_x0, tft_HHMM_y0);

    // redraw the old value to erase
    tft.print(displayedData.timeHHMM);
    // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
  }

  // record location of new HH:MM string on tft display (with background color as this causes a blink)
  tft.getTextBounds(newDisplayData.timeHHMM, 0, 0, &gap_right_x, &gap_up_y, &tft_HHMM_w, &tft_HHMM_h);
  // Serial.print("gap_right_x "); Serial.print(gap_right_x); Serial.print(" gap_up_y "); Serial.print(gap_up_y); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 

  if(screensaver) {
    // set initial values of y0
    if(isThisTheFirstTime) tft_HHMM_y0 = -gap_up_y;
    // move around
    const int16_t adder = 10;
    tft_HHMM_x0 += (screensaverMoveRight ? adder : -adder);
    tft_HHMM_y0 += (screensaverMoveDown ? adder : -adder);
    // set direction
    if(tft_HHMM_x0 <= adder)  screensaverMoveRight = true;
    else if(tft_HHMM_x0 + gap_right_x + tft_HHMM_w >= tft.width() - adder)  screensaverMoveRight = false;
    if(tft_HHMM_y0 + gap_up_y <= adder)  screensaverMoveDown = true;
    else if(tft_HHMM_y0 + gap_up_y + tft_HHMM_h >= tft.height() - adder)  screensaverMoveDown = false;
    // Serial.print("x0 "); Serial.print(tft_HHMM_x0); Serial.print(" y0 "); Serial.println(tft_HHMM_y0);
    // Serial.print("screensaverMoveRight "); Serial.print(screensaverMoveRight); Serial.print(" screensaverMoveDown "); Serial.println(screensaverMoveDown);
    // tft.drawRect(tft_HHMM_x0 + gap_right_x, tft_HHMM_y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
  }

  // home the cursor
  if(!screensaver) {
    tft.setCursor(TIME_ROW_X0, TIME_ROW_Y0);
    // Serial.print("X0 "); Serial.print(TIME_ROW_X0); Serial.print(" Y0 "); Serial.print(TIME_ROW_Y0); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 
  }
  else {
    tft.setCursor(tft_HHMM_x0, tft_HHMM_y0);
    // tft.drawRect(TIME_ROW_X0 + gap_right_x, TIME_ROW_Y0 + gap_up_y, tft_HHMM_w, tft_HHMM_h, Display_Color_White);
    // Serial.print("x0 "); Serial.print(tft_HHMM_x0); Serial.print(" y0 "); Serial.print(tft_HHMM_y0); Serial.print(" w "); Serial.print(tft_HHMM_w); Serial.print(" h "); Serial.println(tft_HHMM_h); 
  }

  // change the text color to foreground color
  if(!screensaver)
    tft.setTextColor(Display_Time_Color);
  else {
    uint16_t random_color = colorPickerWheelBright[random(0,COLOR_PICKER_WHEEL_SIZE - 1)];
    // Serial.print("random_color "); Serial.println(random_color, HEX);
    tft.setTextColor(random_color);
  }

  // draw the new time value
  tft.print(newDisplayData.timeHHMM);
  // tft.setTextSize(1);
  // delay(2000);

  // and remember the new value
  strcpy(displayedData.timeHHMM, newDisplayData.timeHHMM);
}

void rgb_display_class::displayUpdate() {
  bool isThisTheFirstTime = strcmp(displayedData.timeSS, "") == 0;

  // HH:MM string and AM/PM string
  if (strcmp(newDisplayData.timeHHMM, displayedData.timeHHMM) != 0 || redrawDisplay) {

    // HH:MM
    displayHHMM();

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

      // record location of new AM/PM string on tft display (with background color as this causes a blink)
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
    tft.setFont(&FreeSansBold24pt7b);

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
    // record location of new dateStr on tft display (with background color as this causes a blink)
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
    tft.setFont(&FreeSansBold24pt7b);

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
    // record location of new alarmStr on tft display (with background color as this causes a blink)
    tft.getTextBounds(newDisplayData.alarmStr, tft.getCursorX(), tft.getCursorY(), &alarm_row_x0, &alarm_row_y1, &alarm_row_w, &alarm_row_h);
    uint16_t graphic_width = alarm_icon_w + alarm_row_w;
    // three equal length gaps on left center and right of graphic
    uint16_t equal_gaps = (tft.width() - graphic_width) / 3;
    alarm_row_x0 = equal_gaps + alarm_icon_w + equal_gaps;
    alarm_icon_x0 = equal_gaps;
    // align bell center with text center
    alarm_icon_y0 = ALARM_ROW_Y0 - alarm_row_h / 2 - alarm_icon_h / 2;

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
  extern bool refreshRtcTime;
  refreshRtcTime = true;
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
