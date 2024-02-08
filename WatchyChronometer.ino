#define SCREEN_TYPE C

#ifndef SCREEN_TYPE
#define SCREEN_TYPE BW
#endif

#include "WatchyChronometer.h"
#include "settings.h"

WatchyChron watchy(settings);

void setup(){
  watchy.init();
}

void loop(){}