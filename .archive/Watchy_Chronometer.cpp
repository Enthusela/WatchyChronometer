#include "Watchy_Chronometer.h"

#define BORDER_THICKNESS 4
#define DAY_NIGHT_THICKNESS 3
#define CENTRE 0
#define RADIUS 1
#define SUNRISE 2
#define SUNSET 3
#define FIRST_DAY 41
#define LAST_DAY 121
#define ZERO_HOUR 6
#define ZERO_MINUTE 360
#define MINUTES_PER_DAY 1440.0
#define CIRCLE_DEGREES 360.0

const uint8_t DISPLAY_CENTRE_X = DISPLAY_WIDTH / 2;
const uint8_t DISPLAY_CENTRE_Y = DISPLAY_HEIGHT / 2;
uint16_t foregroundColor = GxEPD_BLACK;
uint16_t backgroundColor = GxEPD_WHITE;
uint16_t dayOfYear = 0;
RTC_DATA_ATTR bool showTime = false;
RTC_DATA_ATTR bool showStats = false;
RTC_DATA_ATTR bool darkMode = false;
RTC_DATA_ATTR uint8_t prevDay = 0;

void WatchyChron::drawWatchFace() {
    dayOfYear = monthStartDay[currentTime.Month] + currentTime.Day;
    foregroundColor = darkMode ? GxEPD_BLACK : GxEPD_WHITE;
    backgroundColor = darkMode ? GxEPD_WHITE : GxEPD_BLACK;
    uint8_t currDay = currentTime.Day;
    if (currDay != prevDay) {
      // recalculate sunrise/sunset
      // redraw day/night line
    }
    drawDayNight();
    prevDay = currDay;
    drawSun();
    drawMasks();
    if (showTime) {
        drawTime();
        drawDate();
    }
    if (showStats) {
        drawSteps();
        drawBattery();
    }
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(foregroundColor);
    display.setCursor(DISPLAY_CENTRE_X, DISPLAY_CENTRE_Y - 30);
}

void WatchyChron::drawBattery() {
    const uint8_t BATTERY_ICON_WIDTH = 37;
    const uint8_t BATTERY_SEG_RECT_WIDTH = 27;
    const uint8_t BATTERY_ICON_HEIGHT = 21;
    const uint8_t BATTERY_SEGMENT_WIDTH = 7;
    const uint8_t BATTERY_SEGMENT_HEIGHT = 11;
    const uint8_t BATTERY_SEGMENT_SPACING = 9;
    const uint8_t BATT_POS_X = DISPLAY_CENTRE_X - BATTERY_ICON_WIDTH / 2;
    const uint8_t BATT_POS_Y = 20;

    display.drawBitmap(BATT_POS_X, BATT_POS_Y, battery,
                       BATTERY_ICON_WIDTH, BATTERY_ICON_HEIGHT,
                       foregroundColor);
    display.fillRect(BATT_POS_X + 5, BATT_POS_Y + 5,
                     BATTERY_SEG_RECT_WIDTH, BATTERY_SEGMENT_HEIGHT,
                     backgroundColor); //clear battery segments
    int8_t batteryLevel = 0;
    float VBAT = getBatteryVoltage();
    if(VBAT > 4.1){
        batteryLevel = 3;
    }
    else if(VBAT > 3.95 && VBAT <= 4.1){
        batteryLevel = 2;
    }
    else if(VBAT > 3.80 && VBAT <= 3.95){
        batteryLevel = 1;
    }
    else if(VBAT <= 3.80){
        batteryLevel = 0;
    }

    for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
        display.fillRect(BATT_POS_X + 5 + (batterySegments * BATTERY_SEGMENT_SPACING), BATT_POS_Y + 5,
                         BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT,
                         foregroundColor);
    }
}

void WatchyChron::drawDate() {
    const uint8_t DATE_POS_X = DISPLAY_CENTRE_X;
    const uint8_t DATE_POS_Y = DISPLAY_CENTRE_Y + 45;
    int16_t  x1, y1;
    uint16_t w, h, day_date_offset;

    // Build date string and calculate required offset
    String dayOfWeek = dayStr(currentTime.Wday);
    String month = monthShortStr(currentTime.Month);
    String day = "";
    if(currentTime.Day < 10){
        day = "0";
    }
    day.concat(currentTime.Day);
    String year = (String) tmYearToCalendar(currentTime.Year); // Offset from 1970, since year is stored in uint8_t
    String date = month + " " + day + " " + year;
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(foregroundColor);
    // Centre-align day of week and print
    display.getTextBounds(dayOfWeek, DATE_POS_X, DATE_POS_Y, &x1, &y1, &w, &h);
    display.setCursor(DATE_POS_X - w / 2, DATE_POS_Y);
    display.print(dayOfWeek);
    // Centre-align date and print
    day_date_offset = h + 5;
    display.getTextBounds(date, DATE_POS_X, DATE_POS_Y, &x1, &y1, &w, &h);
    display.setCursor(DATE_POS_X - w / 2 + 50, DATE_POS_Y + day_date_offset);
    display.println(date);
}

void WatchyChron::drawDayNight() {
    display.fillScreen(backgroundColor);
    int dayNightCentre = dayNightLookup[dayOfYear][CENTRE];
    int dayNightRadius = dayNightLookup[dayOfYear][RADIUS];
    int dayNightMaskCentre;
    if (dayNightCentre > 0)
    {
        dayNightMaskCentre = dayNightCentre + DAY_NIGHT_THICKNESS;
    }
    else
    {
        dayNightMaskCentre = dayNightCentre - DAY_NIGHT_THICKNESS;
    }
    display.fillCircle(DISPLAY_WIDTH / 2, dayNightCentre, dayNightRadius, foregroundColor);
    display.fillCircle(DISPLAY_WIDTH / 2, dayNightMaskCentre, dayNightRadius, backgroundColor);
}

void WatchyChron::drawMasks() {
    display.drawBitmap(0, 0, backgroundMask, DISPLAY_WIDTH, DISPLAY_HEIGHT, backgroundColor);
    display.drawBitmap(0, 0, backgroundRing, DISPLAY_WIDTH, DISPLAY_HEIGHT, foregroundColor);
}

void WatchyChron::drawSteps() {
    const uint8_t STEP_ICON_WIDTH = 19;
    const uint8_t STEP_ICON_HEIGHT = 23;
    const uint8_t STEP_POS_X = DISPLAY_CENTRE_X - STEP_ICON_WIDTH - 5;
    const uint8_t STEP_POS_Y = 50;

    // Reset step counter at midnight
    if (currentTime.Hour == 0 && currentTime.Minute == 0){
      sensor.resetStepCounter();
    }
    uint32_t stepCount = sensor.getCounter();
    display.drawBitmap(STEP_POS_X, STEP_POS_Y, steps, STEP_ICON_WIDTH, STEP_ICON_HEIGHT, foregroundColor);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(foregroundColor);
    display.setCursor(DISPLAY_CENTRE_X + 5, STEP_POS_Y + STEP_ICON_HEIGHT - 5);
    display.println(stepCount);
}

void WatchyChron::drawSun() {
    uint16_t sunriseMinute = dayNightLookup[dayOfYear][SUNRISE];
    uint16_t sunsetMinute = dayNightLookup[dayOfYear][SUNSET];
    const uint8_t border_radius = DISPLAY_HEIGHT / 2 - BORDER_THICKNESS / 2;
    const uint8_t sun_icon_width = 65;
    const uint8_t sun_icon_height = 65;
    // Calculate angular pos of current minute relative to 0 (6am)
    float currentMinute = currentTime.Hour * 60 + currentTime.Minute;
    float angle = (currentMinute - ZERO_MINUTE) / MINUTES_PER_DAY * TWO_PI;
    int x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (sun_icon_width / 2);
    int y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (sun_icon_height / 2);
    display.drawBitmap(x_pos, y_pos, sunFill, sun_icon_width, sun_icon_height, foregroundColor);
    if (currentMinute > sunsetMinute || currentMinute < sunriseMinute) {
        display.drawBitmap(x_pos, y_pos, sunBorderNoRays, sun_icon_width, sun_icon_height, foregroundColor);  
    } else {
        display.drawBitmap(x_pos, y_pos, sunBorder, sun_icon_width, sun_icon_height, foregroundColor);
    }
}

void WatchyChron::drawTime() {
    const uint8_t TIME_POS_X = DISPLAY_CENTRE_X;
    const uint8_t TIME_POS_Y = DISPLAY_CENTRE_Y + 25;
    int16_t  x1, y1;
    uint16_t w, h;
    String hour = "", minute = "", time_str = "";

    // Build time string and calculate text bounds    
    if(HOUR_12_24==12){
      hour += ((currentTime.Hour+11)%12)+1;
    } else {
      hour += currentTime.Hour;
    }
    if(currentTime.Hour < 10){
        hour = "0" + hour;
    }
    if(currentTime.Minute < 10){
        minute = "0";
    }
    minute += currentTime.Minute;
    time_str = hour + ":" + minute;
    display.getTextBounds(time_str, TIME_POS_X, TIME_POS_Y, &x1, &y1, &w, &h);
    // Centre-align text and print
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(foregroundColor);
    display.setCursor(TIME_POS_X - w / 2, TIME_POS_Y);
    display.println(time_str);
}

void WatchyChron::handleButtonPress() {
uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
// Menu Button
if (wakeupBit & MENU_BTN_MASK) {
    if (guiState ==
        WATCHFACE_STATE) { // enter menu state if coming from watch face
    showMenu(menuIndex, false);
    } else if (guiState ==
            MAIN_MENU_STATE) { // if already in menu, then select menu item
    switch (menuIndex) {
    case 0:
        showAbout();
        break;
    case 1:
        showBuzz();
        break;
    case 2:
        showAccelerometer();
        break;
    case 3:
        setTime();
        break;
    case 4:
        setupWifi();
        break;
    case 5:
        showUpdateFW();
        break;
    case 6:
        showSyncNTP();
        break;
    default:
        break;
    }
    } else if (guiState == FW_UPDATE_STATE) {
    updateFWBegin();
    }
}
// Back Button
else if (wakeupBit & BACK_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // exit to watch face if already in menu
    RTC.read(currentTime);
    showWatchFace(false);
    } else if (guiState == APP_STATE) {
    showMenu(menuIndex, false); // exit to menu if already in app
    } else if (guiState == FW_UPDATE_STATE) {
    showMenu(menuIndex, false); // exit to menu if already in app
    } else if (guiState == WATCHFACE_STATE) {
        darkMode = !darkMode;
        RTC.read(currentTime);
        showWatchFace(true);
    }
}
// Up Button
else if (wakeupBit & UP_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // increment menu index
    menuIndex--;
    if (menuIndex < 0) {
        menuIndex = MENU_LENGTH - 1;
    }
    showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) { // Toggle stats
        showStats = !showStats;
        RTC.read(currentTime);
        showWatchFace(true);
    }
}
// Down Button
else if (wakeupBit & DOWN_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // decrement menu index
    menuIndex++;
    if (menuIndex > MENU_LENGTH - 1) {
        menuIndex = 0;
    }
    showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) { // Toggle time
        showTime = !showTime;
        RTC.read(currentTime);
        showWatchFace(true);
    }
}

  /***************** fast menu *****************/
  bool timeout     = false;
  long lastTimeout = millis();
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(DOWN_BTN_PIN, INPUT);
  while (!timeout) {
    if (millis() - lastTimeout > 5000) {
      timeout = true;
    } else {
      if (digitalRead(MENU_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState ==
            MAIN_MENU_STATE) { // if already in menu, then select menu item
          switch (menuIndex) {
          case 0:
            showAbout();
            break;
          case 1:
            showBuzz();
            break;
          case 2:
            showAccelerometer();
            break;
          case 3:
            setTime();
            break;
          case 4:
            setupWifi();
            break;
          case 5:
            showUpdateFW();
            break;
          case 6:
            showSyncNTP();
            break;
          default:
            break;
          }
        } else if (guiState == FW_UPDATE_STATE) {
          updateFWBegin();
        }
      } else if (digitalRead(BACK_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState ==
            MAIN_MENU_STATE) { // exit to watch face if already in menu
          RTC.read(currentTime);
          showWatchFace(false);
          break; // leave loop
        } else if (guiState == APP_STATE) {
          showMenu(menuIndex, false); // exit to menu if already in app
        } else if (guiState == FW_UPDATE_STATE) {
          showMenu(menuIndex, false); // exit to menu if already in app
        } else if (guiState == WATCHFACE_STATE) {
          timeout = true;
        }
      } else if (digitalRead(UP_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // increment menu index
          menuIndex--;
          if (menuIndex < 0) {
            menuIndex = MENU_LENGTH - 1;
          }
          showFastMenu(menuIndex);
        } else if (guiState == WATCHFACE_STATE) {
            timeout = true;
        }
      } else if (digitalRead(DOWN_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // decrement menu index
          menuIndex++;
          if (menuIndex > MENU_LENGTH - 1) {
            menuIndex = 0;
          }
          showFastMenu(menuIndex);
        } else if (guiState == WATCHFACE_STATE) {
            timeout = true;
        }
      }
    }
  }
}
