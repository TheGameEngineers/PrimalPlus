#pragma once
#include "../../Common/ImportersCommonHeaders.h"
unsigned char* LoadBMP(const char* filename, int* width, int* height, int* bpp, int desiredChannels);
enum BMPCompressionMethod
{
    BI_RGB = 0,
    BI_RLE8 = 1,
    BI_RLE4 = 2,
    BI_BITFIELDS = 3,
    BI_JPEG = 4,
    BI_PNG = 5,
    BI_ALPHABITFIELDS = 6,
    BI_CMYK = 11,
    BI_CMYKRLE8 = 12,
    BI_CMYKRLE4 = 13
};
BMPCompressionMethod GetBMPCompressionMethod(const char* filename);