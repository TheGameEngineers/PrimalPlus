#pragma once
//#include <cairo/cairo.h>
#include <nanosvg/nanosvgrast.h>
#include "../../Common/ImportersCommonHeaders.h"
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
unsigned char* LoadSVG(const char* filename, int* width, int* height, int* bpp, int desiredChannels);