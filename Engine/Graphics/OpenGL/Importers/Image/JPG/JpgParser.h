#pragma once
#include "../../Common/ImportersCommonHeaders.h"
#include <vcruntime_string.h>
#include <jpeglib.h>
#include <corecrt_malloc.h>
#ifndef JPGPARSER_H
#define JPGPARSER_H

unsigned char* LoadJPG(const char* filename, int* width, int* height, int* bpp, int desiredChannels);

#endif