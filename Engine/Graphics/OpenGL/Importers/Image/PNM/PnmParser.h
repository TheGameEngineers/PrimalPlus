#pragma once
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadPNM(const char* filename, int* width, int* height, int* bpp, int desiredChannels);