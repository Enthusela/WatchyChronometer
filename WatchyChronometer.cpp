#include "WatchyChronometer.h"
#include "icons.h"
#include "moon_bitmaps.h"

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
// First image corresponds to 5pm (last to 7am)
#define ZERO_INDEX_HOUR 17
// First image at 5pm plus two images per hour puts midnight at index 14
#define MIDNIGHT_INDEX 14

const uint8_t DISPLAY_CENTRE_X = DISPLAY_WIDTH / 2;
const uint8_t DISPLAY_CENTRE_Y = DISPLAY_HEIGHT / 2;
uint16_t foregroundColor = GxEPD_BLACK;
uint16_t backgroundColor = GxEPD_WHITE;
uint16_t dayOfYear = 0;
RTC_DATA_ATTR bool showTime = false;
RTC_DATA_ATTR bool showStats = false;
RTC_DATA_ATTR bool darkMode = false;
RTC_DATA_ATTR uint8_t prevDay = 0;

struct xyPoint {
  int x;
  int y;
};

void WatchyChron::drawWatchFace() {
    dayOfYear = monthStartDay[currentTime.Month] + currentTime.Day;
    foregroundColor = darkMode ? GxEPD_BLACK : GxEPD_WHITE;
    backgroundColor = darkMode ? GxEPD_WHITE : GxEPD_BLACK;
    uint8_t currDay = currentTime.Day;
    if (currDay != prevDay) {
      // recalculate sunrise/sunset
      // redraw day/night line
    }
    display.fillScreen(backgroundColor);
    if (!showTime) {
      drawDayNight();
    }
    prevDay = currDay;
    if (!showTime) {
      drawSun();
    }
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


void WatchyChron::drawDayNight() {
    // display.fillScreen(backgroundColor);
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


void WatchyChron::drawSun() {
    uint16_t sunriseMinute = dayNightLookup[dayOfYear][SUNRISE];
    uint16_t sunsetMinute = dayNightLookup[dayOfYear][SUNSET];
    const uint8_t border_radius = DISPLAY_HEIGHT / 2 - BORDER_THICKNESS / 2;
    const uint8_t sun_icon_width = 65;
    const uint8_t sun_icon_height = 65;
    // Calculate angular pos in radians of current minute relative to 0 (6am)
    float currentMinute = currentTime.Hour * 60 + currentTime.Minute;
    float angle = (currentMinute - ZERO_MINUTE) / MINUTES_PER_DAY * TWO_PI;
    int sun_x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (sun_icon_width / 2);
    int sun_y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (sun_icon_height / 2);
    bool daytime = currentMinute >= sunriseMinute && currentMinute < sunsetMinute;
    if (daytime) {
        display.drawBitmap(sun_x_pos, sun_y_pos, sunFill, sun_icon_width, sun_icon_height, foregroundColor);
        display.drawBitmap(sun_x_pos, sun_y_pos, sunBorder, sun_icon_width, sun_icon_height, foregroundColor);
    } else {
        // Moon image index calculated from current time
        uint8_t index;
        if (currentTime.Hour >= ZERO_INDEX_HOUR) {
            index = (currentTime.Hour - ZERO_INDEX_HOUR) * 2;
        } else {
            index = MIDNIGHT_INDEX + (currentTime.Hour * 2);
        }
        if (currentTime.Minute > 15 && currentTime.Minute < 45) {
            index += 1;
        }
        if (currentTime.Minute >= 45) {
            index += 2;
        }
        index = min(index, bmp_moonWax2qrt_array_max_index);
        // positive mask is 24 pixels wide: 
        // don't show for newmoon
        // offset 0 for wax1qrt
        // offset 8 for wax2qrt
        // offset 16 for wax3qrt
        // fullmoon for fullmoon
        // offset 24 for wane3qrt
        // offset 32 for wane2qrt
        // offset 40 for wane1qrt
        uint8_t phase_offset = 0;
        // Calculate moon position
        const uint8_t midnight_x_pos = DISPLAY_CENTRE_X;
        const uint8_t midnight_y_pos = DISPLAY_CENTRE_Y + border_radius;
        const uint8_t moon_phase_add_mask_width = 24;
        const uint8_t moon_phase_add_mask_height = 33;
        uint8_t moon_phase_add_mask_pos_x = midnight_x_pos - moon_phase_add_mask_width + phase_offset;
        uint8_t moon_phase_add_mask_pos_y = midnight_y_pos - (moon_phase_add_mask_height / 2);
        // midnight is -HALF_PI from zero_angle
        xyPoint moon_phase_add_mask_pos = rotatePointAround(moon_phase_add_mask_pos_x,
                                                            moon_phase_add_mask_pos_y,
                                                            DISPLAY_CENTRE_X,
                                                            DISPLAY_CENTRE_Y,
                                                            angle + HALF_PI);
        display.drawBitmap(moon_phase_add_mask_pos.x,
                            moon_phase_add_mask_pos.y,
                            bmp_moonWax2qrt_array[index],
                            moon_phase_add_mask_width,
                            moon_phase_add_mask_height,
                            foregroundColor);

        // Draw moon bitmap
        const uint8_t moon_icon_width = 33;
        const uint8_t moon_icon_height = 33;
        int moon_x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (moon_icon_width / 2);
        int moon_y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (moon_icon_height / 2);
        display.drawBitmap(moon_x_pos, moon_y_pos, bmp_moonWax2qrt_array[index], moon_icon_width, moon_icon_height, foregroundColor);
        display.drawBitmap(sun_x_pos, sun_y_pos, sunBorderNoRays, sun_icon_width, sun_icon_height, foregroundColor);
    }
}


struct xyPoint rotatePointAround(int x, int y, int ox, int oy, double angle) {
  // rotate X,Y point around given origin point by a given angle
  // based on https://gist.github.com/LyleScott/e36e08bfb23b1f87af68c9051f985302#file-rotate_2d_point-py-L38
  double qx = (double)ox + (cos(angle) * (double)(x - ox)) + (sin(angle) * (double)(y - oy));
  double qy = (double)oy + (-sin(angle) * (double)(x - ox)) + (cos(angle) * (double)(y - oy));
  struct xyPoint newPoint;
  newPoint.x = (int)qx;
  newPoint.y = (int)qy;
  return newPoint;
}


void WatchyChron::drawMasks() {
    display.drawBitmap(0, 0, backgroundMask, DISPLAY_WIDTH, DISPLAY_HEIGHT, backgroundColor);
    display.drawBitmap(0, 0, backgroundRing, DISPLAY_WIDTH, DISPLAY_HEIGHT, foregroundColor);
}


void WatchyChron::drawTime() {
    const uint8_t TIME_POS_X = DISPLAY_CENTRE_X;
    const uint8_t TIME_POS_Y = DISPLAY_CENTRE_Y + 25;
    display.setFont(&MADE_Sunflower_PERSONAL_USE39pt7b);
    display.setTextColor(foregroundColor);
    display.setTextWrap(false);
    char* timeStr;
    asprintf(&timeStr, "%d:%02d", currentTime.Hour, currentTime.Minute);
    drawCenteredString(timeStr, TIME_POS_X, TIME_POS_Y, false);
    free(timeStr);
}


void WatchyChron::drawCenteredString(const String &str, int x, int y, bool drawBg) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(str, x, y, &x1, &y1, &w, &h);
    display.setCursor(x - w / 2, y);
    if(drawBg) {
    int padY = 3;
    int padX = 10;
    display.fillRect(x - (w / 2 + padX), y - (h + padY), w + padX*2, h + padY*2, backgroundColor);
    }
    // uncomment to draw bounding box
//          display.drawRect(x - w / 2, y - h, w, h, GxEPD_WHITE);
    display.print(str);
}


void WatchyChron::drawDate() {
    const uint8_t DATE_POS_X = DISPLAY_CENTRE_X;
    const uint8_t WDAY_POS_Y = DISPLAY_CENTRE_Y + 50;
    const uint8_t DATE_POS_Y = WDAY_POS_Y + 20;
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
    drawCenteredString(dayOfWeek, DATE_POS_X, WDAY_POS_Y, false);
    drawCenteredString(date, DATE_POS_X, DATE_POS_Y, false);
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
