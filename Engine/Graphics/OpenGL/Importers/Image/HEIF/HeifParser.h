#pragma once
#include <libheif/heif.h>
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadHEIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels);