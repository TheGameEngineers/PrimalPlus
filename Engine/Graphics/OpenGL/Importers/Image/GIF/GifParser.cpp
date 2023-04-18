#include "GifParser.h"
unsigned char* LoadGIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    int error = 0;
    GifFileType* gif = DGifOpenFileName(filename, &error);
    if (!gif) {
        std::cerr << "Error opening GIF file: " << GifErrorString(error) << std::endl;
        return nullptr;
    }

    if (DGifSlurp(gif) == GIF_ERROR) {
        std::cerr << "Error reading GIF file: " << GifErrorString(gif->Error) << std::endl;
        DGifCloseFile(gif, &error);
        return nullptr;
    }

    *width = gif->SWidth;
    *height = gif->SHeight;
    *bpp = desiredChannels;

    std::vector<unsigned char> imageData(gif->SWidth * gif->SHeight * desiredChannels);

    // TODO: Process the GIF data and fill in the imageData vector

    ColorMapObject* colorMap = (gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap);
    if (!colorMap) {
        std::cerr << "Error: GIF file has no color map" << std::endl;
        DGifCloseFile(gif, &error);
        return nullptr;
    }

    for (int i = 0; i < gif->ImageCount; i++) {
        SavedImage* frame = &gif->SavedImages[i];
        GifImageDesc* frameDesc = &frame->ImageDesc;

        for (int y = 0; y < frameDesc->Height; y++) {
            for (int x = 0; x < frameDesc->Width; x++) {
                int index = frame->RasterBits[y * frameDesc->Width + x];
                GifColorType* color = &colorMap->Colors[index];

                int pixelIndex = ((frameDesc->Top + y) * gif->SWidth + (frameDesc->Left + x)) * desiredChannels;
                imageData[pixelIndex + 0] = color->Red;
                imageData[pixelIndex + 1] = color->Green;
                imageData[pixelIndex + 2] = color->Blue;
                imageData[pixelIndex + 3] = 255; // Set alpha to fully opaque
            }
        }
    }

    // Flip the image data vertically
    int rowSize = gif->SWidth * desiredChannels;
    unsigned char* rowBuffer = new unsigned char[rowSize];

    for (int y = 0; y < gif->SHeight / 2; y++) {
        unsigned char* row1 = &imageData[y * rowSize];
        unsigned char* row2 = &imageData[(gif->SHeight - 1 - y) * rowSize];

        std::memcpy(rowBuffer, row1, rowSize);
        std::memcpy(row1, row2, rowSize);
        std::memcpy(row2, rowBuffer, rowSize);
    }

    delete[] rowBuffer;

    DGifCloseFile(gif, &error);

    unsigned char* data = new unsigned char[imageData.size()];
    std::copy(imageData.begin(), imageData.end(), data);
    return data;
}