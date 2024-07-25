#include "WatchyChronometer.h"
#include "bmp_moon.h"

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
#define ZERO_INDEX_HOUR 17
// Must be a factor of 15
// 3 degrees per image: MIN_PER_IMAGE = 60 / (15 / DEGREES_PER_IMAGE) = 12
#define MIN_PER_IMAGE 12
#define MAX_IMAGE_INDEX 75
#define SUN_ICON_WIDTH 65
#define SUN_ICON_HEIGHT 65
#define MOON_ICON_WIDTH 32
#define MOON_ICON_HEIGHT 32
#define NEW_MOON_PHASE_INDEX 0
#define FULL_MOON_PHASE_INDEX 4
// Datum New Moon at 06/07/2024 (https://www.perthobservatory.com.au/wp-content/uploads/moon-phases-2024.pdf)
// Day of year = 6 + 182 (start day of July 2024, from monthStartDayLeapYear[])
#define DATUM_NEW_MOON_DAY_OF_YEAR 188
#define DATUM_NEW_MOON_YEAR 2024

const uint8_t DISPLAY_CENTRE_X = DISPLAY_WIDTH / 2;
const uint8_t DISPLAY_CENTRE_Y = DISPLAY_HEIGHT / 2;
uint16_t foregroundColor = GxEPD_BLACK;
uint16_t backgroundColor = GxEPD_WHITE;
uint16_t dayOfYear = 0;
RTC_DATA_ATTR bool showDigitalTime = false;
RTC_DATA_ATTR bool showStats = false;
RTC_DATA_ATTR bool darkMode = false;
RTC_DATA_ATTR uint8_t prevDay = 0;

void WatchyChron::drawWatchFace() {
    if (tmYearToCalendar(currentTime.Year) % 4) {
        dayOfYear = monthStartDay[currentTime.Month] + currentTime.Day;
    } else {
        dayOfYear = monthStartDayLeapYear[currentTime.Month] + currentTime.Day;
    }
    foregroundColor = darkMode ? GxEPD_BLACK : GxEPD_WHITE;
    backgroundColor = darkMode ? GxEPD_WHITE : GxEPD_BLACK;
    display.fillScreen(backgroundColor);
    if (!showDigitalTime) {
      drawSunriseSunsetLine();
      drawSundialTime();
    }
    drawMasks();
    if (showDigitalTime) {
        drawDigitalTime();
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


void WatchyChron::drawSunriseSunsetLine() {
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


void WatchyChron::drawSundialTime() {
    const uint16_t sunriseMinute = dayNightLookup[dayOfYear][SUNRISE];
    const uint16_t sunsetMinute = dayNightLookup[dayOfYear][SUNSET];
    const uint8_t border_radius = DISPLAY_HEIGHT / 2 - BORDER_THICKNESS / 2;
    const float currentMinute = currentTime.Hour * 60 + currentTime.Minute;
    const float angle = (currentMinute - ZERO_MINUTE) / MINUTES_PER_DAY * TWO_PI;
    const int sun_x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (SUN_ICON_WIDTH / 2);
    const int sun_y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (SUN_ICON_HEIGHT / 2);
    
    const bool daytime = currentMinute >= sunriseMinute && currentMinute < sunsetMinute;
    if (daytime) { // Draw sun
        display.drawBitmap(sun_x_pos, sun_y_pos, sunFill, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, foregroundColor);
        display.drawBitmap(sun_x_pos, sun_y_pos, sunBorder, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, foregroundColor);
    } else { // Draw moon
        int moon_x_pos = DISPLAY_CENTRE_X + border_radius * cos(angle) - (MOON_ICON_WIDTH / 2);
        int moon_y_pos = DISPLAY_CENTRE_Y - border_radius * sin(angle) - (MOON_ICON_HEIGHT / 2);
        const uint8_t moonPhase = getMoonPhase();
        if (moonPhase == NEW_MOON_PHASE_INDEX) {
            display.drawBitmap(sun_x_pos, sun_y_pos, sunFill, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, GxEPD_BLACK);
        } else if (moonPhase == FULL_MOON_PHASE_INDEX) {
            display.drawBitmap(sun_x_pos, sun_y_pos, sunFill, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, GxEPD_WHITE);
        } else {
            const uint8_t index = getMoonImageIndex();
            display.drawBitmap(sun_x_pos, sun_y_pos, sunFill, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, GxEPD_BLACK);
            display.drawBitmap(moon_x_pos, moon_y_pos, bmp_moon_array[moonPhase][index], MOON_ICON_WIDTH, MOON_ICON_HEIGHT, GxEPD_WHITE);
        }
        // Moon fill is white on black, but border matches light/dark theme
        display.drawBitmap(sun_x_pos, sun_y_pos, moonBorder, SUN_ICON_WIDTH, SUN_ICON_HEIGHT, foregroundColor);
    }
}


uint8_t WatchyChron::getMoonPhase() {
    const uint8_t yearDiff = tmYearToCalendar(currentTime.Year) - DATUM_NEW_MOON_YEAR;
    uint16_t daysSinceDatumNewMoon = 0;
    if (dayOfYear >= DATUM_NEW_MOON_DAY_OF_YEAR) {
        daysSinceDatumNewMoon = (dayOfYear - DATUM_NEW_MOON_DAY_OF_YEAR) + (365 * yearDiff);
    } else {
        // Current time must be after datum, so yearDiff >= 1 if currentDayOfYear < datumDayOfYear
        // Datum New Moon must be in the past for this part of the calculation to work
        daysSinceDatumNewMoon = (365 - DATUM_NEW_MOON_DAY_OF_YEAR) + dayOfYear + (365 * (yearDiff - 1));
    }
    // Lunar cycle = 29.5 days, 8 phases, 29.5 / 8 ~3.688, 100x everything to enable modulo
    // return min(7, ((daysSinceDatumNewMoon * 100) % 2950) / 369);
    return min(uint8_t(7), uint8_t(round(float((daysSinceDatumNewMoon * 100) % 2953) / 368.8)));
}


uint16_t WatchyChron::getMoonImageIndex() {
    const uint16_t minutesSinceMidnight = currentTime.Hour * 60 + currentTime.Minute;
    uint16_t minutesSinceFirstImage = 0;
    if (currentTime.Hour >= ZERO_INDEX_HOUR) {
        minutesSinceFirstImage = minutesSinceMidnight - (ZERO_INDEX_HOUR * 60);
    } else {
        minutesSinceFirstImage = ((24 - ZERO_INDEX_HOUR) * 60) + minutesSinceMidnight;
    }
    uint16_t index = (minutesSinceFirstImage + (MIN_PER_IMAGE / 2)) / MIN_PER_IMAGE;
    index > MAX_IMAGE_INDEX ? MAX_IMAGE_INDEX : index;
    
    return index;
}


void WatchyChron::drawMasks() {
    display.drawBitmap(0, 0, backgroundMask, DISPLAY_WIDTH, DISPLAY_HEIGHT, backgroundColor);
    display.drawBitmap(0, 0, backgroundRing, DISPLAY_WIDTH, DISPLAY_HEIGHT, foregroundColor);
}


void WatchyChron::drawDigitalTime() {
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
    if (currentTime.Hour == 0 && currentTime.Minute == 0) {
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
        if (guiState == WATCHFACE_STATE) { // enter menu state if coming from watch face
            showMenu(menuIndex, false);
        } else if (guiState == MAIN_MENU_STATE) { // if already in menu, then select menu item
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
            showDigitalTime = !showDigitalTime;
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
                if (guiState == MAIN_MENU_STATE) { // if already in menu, then select menu item
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