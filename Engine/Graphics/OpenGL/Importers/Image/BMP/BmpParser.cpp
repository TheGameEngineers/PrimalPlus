#include "BmpParser.h"
unsigned char* LoadBMP(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    BMPCompressionMethod compressionMethod = GetBMPCompressionMethod(filename);
    if (compressionMethod != BMPCompressionMethod::BI_RGB) {
        std::cerr << "File is compressed and is not currently supported." << std::endl;
        return nullptr;
    }
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    unsigned char* data;

    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return nullptr;
    }

    file.read((char*)header, 54);
    if (file.gcount() != 54)
    {
        std::cerr << "Not a valid BMP file (header size mismatch)" << std::endl;
        return nullptr;
    }

    if (header[0] != 'B' || header[1] != 'M')
    {
        std::cerr << "Not a valid BMP file (signature mismatch)" << std::endl;
        return nullptr;
    }

    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    *width = *(int*)&(header[0x12]);
    *height = *(int*)&(header[0x16]);
    *bpp = *(int*)&(header[0x1C]);

    if (*width <= 0 || *height <= 0)
    {
        std::cerr << "Not a valid BMP file (invalid dimensions)" << std::endl;
        return nullptr;
    }

    if (*bpp != 24 && *bpp != 32)
    {
        std::cerr << "Unsupported BMP format (only 24-bit and 32-bit supported)" << std::endl;
        return nullptr;
    }

    if (imageSize == 0) imageSize = (*width) * (*height) * desiredChannels;
    if (dataPos == 0) dataPos = 54;

    data = new unsigned char[imageSize];
    file.seekg(dataPos);
    file.read((char*)data, imageSize);
    if (file.gcount() != imageSize)
    {
        std::cerr << "Not a valid BMP file (image data size mismatch)" << std::endl;
        delete[] data;
        return nullptr;
    }
    file.close()    ;

    // Convert BGR to RGBA
    int numChannels = (*bpp) / 8;
    int rowStride = ((*width) * numChannels + 3) & ~3;
    unsigned char* rgbaData = new unsigned char[(*width) * (*height) * 4];
    for (int y = 0; y < (*height); y++)
    {
        for (int x = 0; x < (*width); x++)
        {
            int i = y * rowStride + x * numChannels;
            int j = y * (*width) + x;
            rgbaData[j * 4 + 0] = data[i + 2];
            rgbaData[j * 4 + 1] = data[i + 1];
            rgbaData[j * 4 + 2] = data[i + 0];
            rgbaData[j * 4 + 3] = (numChannels == 4) ? data[i + 3] : 255;
        }
    }

    delete[] data;

    return rgbaData;
}

BMPCompressionMethod GetBMPCompressionMethod(const char* filename)
{
    unsigned char header[54];
    unsigned int compression;

    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return BMPCompressionMethod::BI_RGB;
    }

    file.read((char*)header, 54);
    if (file.gcount() != 54)
    {
        std::cerr << "Not a valid BMP file (header size mismatch)" << std::endl;
        return BMPCompressionMethod::BI_RGB;
    }

    if (header[0] != 'B' || header[1] != 'M')
    {
        std::cerr << "Not a valid BMP file (signature mismatch)" << std::endl;
        return BMPCompressionMethod::BI_RGB;
    }

    compression = *(int*)&(header[0x1E]);
    return (BMPCompressionMethod)compression;
}
// TODO: Add decompression algorithm for BMP.