#pragma once
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadPPM(const char* filename, int* width, int* height, int* bpp, int desiredChannels);
unsigned char* ConvertImageFormat(const unsigned char* data, int width, int height, int srcNumChannels, int dstNumChannels);
unsigned char* LoadPPM_P3(std::ifstream& file, int* width, int* height, int* bpp, int desiredChannels, std::string line);
unsigned char* LoadPPM_P6(std::ifstream& file, int* width, int* height, int* bpp, int desiredChannels, std::string line);
void ToggleInversion(bool value);
