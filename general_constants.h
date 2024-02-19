#ifndef GENERAL_CONSTANTS_H
#define GENERAL_CONSTANTS_H

// max user inactivity seconds before turning down brightness or turning On screensaver
const uint8_t kInactivitySecondsLimit = 120;

// char array sizes for time, date and alarm texts
const int kHHMM_ArraySize = 6, kSS_ArraySize = 4, kDateArraySize = 13, kAlarmArraySize = 10;

// day arrays
#define DAY_ARR_SIZE 4
const char kDaySun[DAY_ARR_SIZE] = "Sun";
const char kDayMon[DAY_ARR_SIZE] = "Mon";
const char kDayTue[DAY_ARR_SIZE] = "Tue";
const char kDayWed[DAY_ARR_SIZE] = "Wed";
const char kDayThu[DAY_ARR_SIZE] = "Thu";
const char kDayFri[DAY_ARR_SIZE] = "Fri";
const char kDaySat[DAY_ARR_SIZE] = "Sat";

// Then set up a table to refer to your strings.
const char *const kDaysTable_[7] = { kDaySun, kDayMon, kDayTue, kDayWed, kDayThu, kDayFri, kDaySat };

// day arrays
#define MONTH_ARR_SIZE 4
const char kMonthJan[MONTH_ARR_SIZE] = "Jan";
const char kMonthFeb[MONTH_ARR_SIZE] = "Feb";
const char kMonthMar[MONTH_ARR_SIZE] = "Mar";
const char kMonthApr[MONTH_ARR_SIZE] = "Apr";
const char kMonthMay[MONTH_ARR_SIZE] = "May";
const char kMonthJun[MONTH_ARR_SIZE] = "Jun";
const char kMonthJul[MONTH_ARR_SIZE] = "Jul";
const char kMonthAug[MONTH_ARR_SIZE] = "Aug";
const char kMonthSep[MONTH_ARR_SIZE] = "Sep";
const char kMonthOct[MONTH_ARR_SIZE] = "Oct";
const char kMonthNov[MONTH_ARR_SIZE] = "Nov";
const char kMonthDec[MONTH_ARR_SIZE] = "Dec";

// Then set up a table to refer to your strings.
const char *const kMonthsTable[12] = { kMonthJan, kMonthFeb, kMonthMar, kMonthApr, kMonthMay, kMonthJun, kMonthJul, kMonthAug, kMonthSep, kMonthOct, kMonthNov, kMonthDec };

const char kAmLabel[3] = "AM", kPmLabel[3] = "PM", kOffLabel[4] = "Off", kAlarmLabel[6] = "Alarm";
const char kCharSpace = ' ', kCharZero = '0';

const unsigned int kWifiSsidPasswordLengthMax = 32;

#endif  // GENERAL_CONSTANTS_H