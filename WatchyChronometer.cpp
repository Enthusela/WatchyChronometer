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
RTC_DATA_ATTR int listIndex;
RTC_DATA_ATTR uint8_t prevDay = 0;
const char *listItems[] = {
    //  "---------------"
        "3 red capsicum",
        "400g mushroom",
        "1.5kg chicken",
        "One pc garlic",
        "1 pkt salad",
        "3 tins beans",
        "500ml Ckn Stock",
        "3 red capsicum",
        "400g mushroom",
        "1.5kg chicken",
        "One pc garlic",
        "1 pkt salad",
        "3 tins beans",
        "500ml Ckn Stock",
    };
const uint8_t listLen = 14; // Constant for now while I'm hashing this out
bool listChecks[listLen];

struct xyPoint {
  int x;
  int y;
};

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
    if (dayNightCentre > 0) {
        dayNightMaskCentre = dayNightCentre + DAY_NIGHT_THICKNESS;
    }
    else {
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
        // Draw moon bitmap
        const uint8_t moon_icon_width = 33;
        const uint8_t moon_icon_height = 33;
        int moon_x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (moon_icon_width / 2);
        int moon_y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (moon_icon_height / 2);
        display.drawBitmap(moon_x_pos, moon_y_pos, bmp_moonWax2qrt_array[index], moon_icon_width, moon_icon_height, foregroundColor);
        display.drawBitmap(sun_x_pos, sun_y_pos, sunBorderNoRays, sun_icon_width, sun_icon_height, foregroundColor);
    }
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
          showShoppingList(0, false);
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
      } else if (guiState == SHOPLIST_STATE) {
        showMenu(menuIndex, false); // exit to menu if in shopping list
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
      } else if (guiState == SHOPLIST_STATE) {  // increment list index (index decr, selection moves up screen)
        listIndex--;
        if (listIndex < 0) {
            listIndex = listLen;
        }
        showShoppingList(listIndex, true);
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
      } else if (guiState == SHOPLIST_STATE) {  // decrement list index (index incr, selection moves down screen)
        listIndex++;
        if (listIndex >= listLen) {
            listIndex = 0;
        }
        showShoppingList(listIndex, true);
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
            showShoppingList(0, false);
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
        if (guiState == MAIN_MENU_STATE) { // exit to watch face if already in menu
          RTC.read(currentTime);
          showWatchFace(false);
          break; // leave loop
        } else if (guiState == APP_STATE) {
          showMenu(menuIndex, false); // exit to menu if already in app
        } else if (guiState == FW_UPDATE_STATE) {
          showMenu(menuIndex, false); // exit to menu if already in app
        } else if (guiState == WATCHFACE_STATE) {
          timeout = true;
        } else if (guiState == SHOPLIST_STATE) {
          showMenu(menuIndex, false); // exit to menu if in shopping list
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
        } else if (guiState == SHOPLIST_STATE) {  // increment list index (index decr, selection moves up screen)
          listIndex--;
          if (listIndex < 0) {
              listIndex = listLen;
          }
          showShoppingList(listIndex, true);
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
        } else if (guiState == SHOPLIST_STATE) {  // increment list index (index decr, selection moves up screen)
          listIndex--;
          if (listIndex < 0) {
            listIndex = listLen;
          }
          showShoppingList(listIndex, true);
        }
      }
    }
  }
}

void WatchyChron::showMenu(byte menuIndex, bool partialRefresh) {
  display.setFullWindow();
  display.fillScreen(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  int16_t yPos;

  const char *menuItems[] = {
      "About Watchy", "Shopping List", "Show Accelerometer",
      "Set Time",     "Setup WiFi",    "Update Firmware",
      "Sync NTP"};
  for (int i = 0; i < MENU_LENGTH; i++) {
    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);
    display.setCursor(0, yPos);
    if (i == menuIndex) {
      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);
      display.println(menuItems[i]);
    } else {
      display.setTextColor(GxEPD_WHITE);
      display.println(menuItems[i]);
    }
  }

  display.display(partialRefresh);

  guiState = MAIN_MENU_STATE;
  alreadyInMenu = false;
}

void WatchyChron::showFastMenu(byte menuIndex) {
  display.setFullWindow();
  display.fillScreen(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  int16_t yPos;

  const char *menuItems[] = {
      "About Watchy", "Shopping List", "Show Accelerometer",
      "Set Time",     "Setup WiFi",    "Update Firmware",
      "Sync NTP"};
  for (int i = 0; i < MENU_LENGTH; i++) {
    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);
    display.setCursor(0, yPos);
    if (i == menuIndex) {
      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_WHITE);
      display.setTextColor(GxEPD_BLACK);
      display.println(menuItems[i]);
    } else {
      display.setTextColor(GxEPD_WHITE);
      display.println(menuItems[i]);
    }
  }

  display.display(true);

  guiState = MAIN_MENU_STATE;
}

// TODO: showMenu, showShoppingList, showFastMenu, and showShoppingList are near-identical:
    // Investigate consolidating code into fn/wrappers
void WatchyChron::showShoppingList(byte listIndex, bool partialRefresh) {
    static uint16_t lastListSttIndex = 0;
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t x1, y1;
    uint16_t w, h;
    int16_t yPos;

    const uint8_t maxItemLen = 18; // max chars in item not including null char
    // const uint8_t maxItemLenExclCheckbox = 15; // assuming two-char checkbox plus space
    const uint16_t listSttIndex = (listIndex / MENU_LENGTH) * MENU_LENGTH;
    if (listSttIndex != lastListSttIndex) {
        // Paged up or down: do a full refresh
        partialRefresh = false;
    }
    lastListSttIndex = listSttIndex;

    for (int i = listSttIndex; i < listSttIndex + MENU_LENGTH && i < listLen; i++) {
        yPos = MENU_HEIGHT + (MENU_HEIGHT * i);
        display.setCursor(0, yPos);
        if (i == listIndex) {
            display.getTextBounds(listItems[i], 0, yPos, &x1, &y1, &w, &h);
            display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_WHITE); // 200 might just be display width
            display.setTextColor(GxEPD_BLACK);
            display.println(listItems[i]);
        } else {
            display.setTextColor(GxEPD_WHITE);
            display.println(listItems[i]);
        }
        if (listChecks[i]) {
            display.drawLine(x1 - 1, y1 + h/2, 200, y1 + h/2, i == listIndex ? GxEPD_BLACK : GxEPD_WHITE);
        }
    }
    display.display(partialRefresh);
    guiState = SHOPLIST_STATE;
    // Prevent exiting to watchface when in shopping list
    alreadyInMenu = false;
}
