#include "TifParser.h"
std::string lastErrorMessage;

void errorHandler(const char* module, const char* fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    lastErrorMessage = buf;
}

unsigned char* LoadTIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    TIFFSetErrorHandler(errorHandler);
    TIFF* tif = TIFFOpen(filename, "r");
    unsigned char* imageData = NULL;
    if (tif) {
        uint32_t w, h;
        uint16_t bps, spp;
        size_t npixels;
        uint32_t* raster;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
        npixels = w * h;
        raster = (uint32_t*)_TIFFmalloc(npixels * sizeof(uint32_t));
        if (raster != NULL) {
            if (TIFFReadRGBAImage(tif, w, h, raster, 0)) {
                // process image
                imageData = new unsigned char[npixels * desiredChannels];
                for (size_t i = 0; i < npixels; i++) {
                    uint32_t pixel = raster[i];
                    imageData[i * desiredChannels + 0] = TIFFGetR(pixel);
                    imageData[i * desiredChannels + 1] = TIFFGetG(pixel);
                    imageData[i * desiredChannels + 2] = TIFFGetB(pixel);
                    if (desiredChannels == 4) {
                        imageData[i * desiredChannels + 3] = TIFFGetA(pixel);
                    }
                }
                // update width, height and bpp
                *width = w;
                *height = h;
                *bpp = bps * spp;
            }
            else {
                std::cerr << "Error: Could not read image data from TIFF file." << std::endl;
            }
            _TIFFfree(raster);
        }
        else {
            std::cerr << "Error: Could not allocate memory for raster data." << std::endl;
        }
        TIFFClose(tif);
    }
    else {
        std::cerr << "Error: Could not open TIFF file: " << lastErrorMessage << std::endl;
    }
    return imageData;
}