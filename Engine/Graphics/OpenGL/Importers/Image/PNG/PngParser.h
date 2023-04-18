#pragma once
#include "../../Common/ImportersCommonHeaders.h"
#include <png.h>
#include <stdlib.h>
unsigned char* LoadPNG(const char* filename, int* width, int* height, int* bpp, int desiredChannels);
void ToggleFlipped(bool value);
