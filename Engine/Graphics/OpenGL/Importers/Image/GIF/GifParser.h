#pragma once
#include <gif_lib.h>
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadGIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels);