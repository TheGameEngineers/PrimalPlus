#pragma once
#include "../../Common/ImportersCommonHeaders.h"
#include <webp/decode.h>
unsigned char* LoadWebP(const char* filename, int* width, int* height, int* bpp, int desiredChannels);