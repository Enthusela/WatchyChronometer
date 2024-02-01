#ifndef SCREEN_TYPE
#define SCREEN_TYPE BW
#endif

#define SCREEN_TYPE C

#if SCREEN_TYPE == C
  #include "Watchy_Chron_3C.h"
#else
  #include "Watchy_Chronometer.h"
#endif

#include "settings.h"

WatchyChron watchy(settings);

void setup(){
  watchy.init();
}

void loop(){}
