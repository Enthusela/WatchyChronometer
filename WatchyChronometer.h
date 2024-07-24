#ifndef WATCHY_CHRON_H
#define WATCHY_CHRON_H

#include <Watchy.h>
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include "MadeSunflower39pt7b.h"
#include "lookups.h"

#define SHOPLIST_STATE 10 // Start custom states from 10 to allow room for official updates

class WatchyChron : public Watchy{
    using Watchy::Watchy;
    public:
        void drawWatchFace();
        void drawBattery();
        void drawDate();
        void drawDayNight();
        void drawMasks();
        void drawSteps();
        void drawSun();
        void drawTime();
        void drawCenteredString(const String &str, int x, int y, bool drawBg);
        void handleButtonPress();
};

extern RTC_DATA_ATTR bool showTime;
extern RTC_DATA_ATTR bool showStats;
extern RTC_DATA_ATTR bool darkMode;

#endif
