#include "TgaParser.h"
unsigned char* LoadTGA(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    // Open the file
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0)
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return nullptr;
    }

    // Read the TGA header
    unsigned char header[18];
    fread(header, sizeof(unsigned char), 18, file);

    // Get the image information
    *width = header[12] + (header[13] << 8);
    *height = header[14] + (header[15] << 8);
    *bpp = header[16];

    // Calculate the size of the image data
    int imageSize = *width * *height * desiredChannels;

    // Allocate memory for the image data
    unsigned char* imageData = new unsigned char[imageSize];

    // Check if the image is RLE compressed
    if (header[2] == 10)
    {
        // RLE compressed image
        int currentByte = 0;
        while (currentByte < imageSize)
        {
            // Read the RLE packet header
            unsigned char rleHeader;
            fread(&rleHeader, sizeof(unsigned char), 1, file);

            if (rleHeader < 128)
            {
                // Raw packet
                int packetSize = rleHeader + 1;
                for (int i = 0; i < packetSize; i++)
                {
                    unsigned char pixel[4];
                    fread(pixel, sizeof(unsigned char), *bpp / 8, file);

                    // Convert the pixel to the desired format
                    if (*bpp == 8)
                    {
                        // Grayscale to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[0];
                            imageData[currentByte + 1] = pixel[0];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[0];
                            imageData[currentByte + 1] = pixel[0];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = 255;
                        }
                    }
                    else if (*bpp == 16)
                    {
                        // 16-bit to desired format
                        unsigned short color = pixel[0] + (pixel[1] << 8);
                        unsigned char r = (color & 0x7C00) >> 10;
                        unsigned char g = (color & 0x03E0) >> 5;
                        unsigned char b = color & 0x001F;
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = r << 3;
                            imageData[currentByte + 1] = g << 3;
                            imageData[currentByte + 2] = b << 3;
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = r << 3;
                            imageData[currentByte + 1] = g << 3;
                            imageData[currentByte + 2] = b << 3;
                            imageData[currentByte + 3] = (color & 0x8000) ? 255 : 0;
                        }
                    }
                    if (*bpp == 24)
                    {
                        // BGR to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = 255;
                        }
                    }
                    else if (*bpp == 32)
                    {
                        // BGRA to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = pixel[3];
                        }
                    }

                    currentByte += desiredChannels;
                }
            }
            else
            {
                // RLE packet
                int packetSize = rleHeader - 127;
                unsigned char pixel[4];
                fread(pixel, sizeof(unsigned char), *bpp / 8, file);

                for (int i = 0; i < packetSize; i++)
                {
                    // Convert the pixel to the desired format
                    if (*bpp == 8)
                    {
                        // Grayscale to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[0];
                            imageData[currentByte + 1] = pixel[0];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[0];
                            imageData[currentByte + 1] = pixel[0];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = 255;
                        }
                    }
                    else if (*bpp == 16)
                    {
                        // 16-bit to desired format
                        unsigned short color = pixel[0] + (pixel[1] << 8);
                        unsigned char r = (color & 0x7C00) >> 10;
                        unsigned char g = (color & 0x03E0) >> 5;
                        unsigned char b = color & 0x001F;
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = r << 3;
                            imageData[currentByte + 1] = g << 3;
                            imageData[currentByte + 2] = b << 3;
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = r << 3;
                            imageData[currentByte + 1] = g << 3;
                            imageData[currentByte + 2] = b << 3;
                            imageData[currentByte + 3] = (color & 0x8000) ? 255 : 0;
                        }
                    }
                    if (*bpp == 24)
                    {
                        // BGR to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = 255;
                        }
                    }
                    else if (*bpp == 32)
                    {
                        // BGRA to desired format
                        if (desiredChannels == 3)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                        }
                        else if (desiredChannels == 4)
                        {
                            imageData[currentByte] = pixel[2];
                            imageData[currentByte + 1] = pixel[1];
                            imageData[currentByte + 2] = pixel[0];
                            imageData[currentByte + 3] = pixel[3];
                        }
                    }

                    currentByte += desiredChannels;
                }
            }
        }
    }
    else
    {
        // Uncompressed image
        for (int i = 0; i < imageSize; i += desiredChannels)
        {
            unsigned char pixel[4];
            fread(pixel, sizeof(unsigned char), *bpp / 8, file);
            // Convert the pixel to the desired format
            if (*bpp == 8)
            {
                // Grayscale to desired format
                if (desiredChannels == 3)
                {
                    imageData[i] = pixel[0];
                    imageData[i + 1] = pixel[0];
                    imageData[i + 2] = pixel[0];
                }
                else if (desiredChannels == 4)
                {
                    imageData[i] = pixel[0];
                    imageData[i + 1] = pixel[0];
                    imageData[i + 2] = pixel[0];
                    imageData[i + 3] = 255;
                }
            }
            else if (*bpp == 16)
            {
                // 16-bit to desired format
                unsigned short color = pixel[0] + (pixel[1] << 8);
                unsigned char r = (color & 0x7C00) >> 10;
                unsigned char g = (color & 0x03E0) >> 5;
                unsigned char b = color & 0x001F;
                if (desiredChannels == 3)
                {
                    imageData[i] = r << 3;
                    imageData[i + 1] = g << 3;
                    imageData[i + 2] = b << 3;
                }
                else if (desiredChannels == 4)
                {
                    imageData[i] = r << 3;
                    imageData[i + 1] = g << 3;
                    imageData[i + 2] = b << 3;
                    imageData[i + 3] = (color & 0x8000) ? 255 : 0;
                }
            }
            if (*bpp == 24)
            {
                // BGR to desired format
                if (desiredChannels == 3)
                {
                    imageData[i] = pixel[2];
                    imageData[i + 1] = pixel[1];
                    imageData[i + 2] = pixel[0];
                }
                else if (desiredChannels == 4)
                {
                    imageData[i] = pixel[2];
                    imageData[i + 1] = pixel[1];
                    imageData[i + 2] = pixel[0];
                    imageData[i + 3] = 255;
                }
            }
            else if (*bpp == 32)
            {
                // BGRA to desired format
                if (desiredChannels == 3)
                {
                    imageData[i] = pixel[2];
                    imageData[i + 1] = pixel[1];
                    imageData[i + 2] = pixel[0];
                }
                else if (desiredChannels == 4)
                {
                    imageData[i] = pixel[2];
                    imageData[i + 1] = pixel[1];
                    imageData[i + 2] = pixel[0];
                    imageData[i + 3] = pixel[3];
                }
            }
        }
    }

    // Close the file
    fclose(file);

    return imageData;
}