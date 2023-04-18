#pragma once
#include <tiffio.h>
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadTIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels);