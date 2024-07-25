#ifndef BMP_MOON_H
#define BMP_MOON_H

#include "bmp_moonWax1qrt.h"
#include "bmp_moonWax2qrt.h"
#include "bmp_moonWax3qrt.h"
#include "bmp_moonWane3qrt.h"
#include "bmp_moonWane2qrt.h"
#include "bmp_moonWane1qrt.h"

const unsigned char** bmp_moon_array[8] = {
    NULL,
    bmp_moonWax1qrt_array,
    bmp_moonWax2qrt_array,
    bmp_moonWax3qrt_array,
    NULL,
    bmp_moonWane3qrt_array,
    bmp_moonWane2qrt_array,
    bmp_moonWane1qrt_array,
};

#endif