#include "PgmParser.h"
bool invert = false;
void SetInversion(bool value) {
    invert = value;
}
unsigned char* LoadPGM(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;
    std::getline(file, line);
    bool isBinary = (line == "P5");
    if (!isBinary && line != "P2")
    {
        std::cerr << "Invalid PGM file: " << filename << std::endl;
        return nullptr;
    }

    std::getline(file, line);
    while (line[0] == '#')
        std::getline(file, line);

    std::stringstream ss(line);
    ss >> *width >> *height;

    if (*width <= 0 || *height <= 0)
    {
        std::cerr << "Invalid image dimensions: " << *width << "x" << *height << std::endl;
        return nullptr;
    }

    std::getline(file, line);
    ss.str(line);
    ss.clear();
    ss >> *bpp;

    if (*bpp <= 0 || *bpp > 65535)
    {
        std::cerr << "Invalid maximum pixel value: " << *bpp << std::endl;
        return nullptr;
    }

    int size = *width * *height * desiredChannels;
    unsigned char* data = new unsigned char[size];

    float scale = 255.0f / *bpp;

    if (isBinary)
    {
        int rowSize = *width * (*bpp <= 255 ? 1 : 2);
        std::vector<char> buffer(rowSize);

        for (int y = 0; y < *height; ++y)
        {
            int row = invert ? (*height - 1 - y) : y;

            file.read(buffer.data(), rowSize);
            if (!file)
            {
                std::cerr << "Failed to read pixel data from file" << std::endl;
                delete[] data;
                return nullptr;
            }

            const char* rowData = buffer.data();
            for (int x = 0; x < *width; ++x)
            {
                unsigned short pixel = *bpp <= 255 ? (unsigned char)*rowData++ : (unsigned short)(*(unsigned short*)rowData)++;
                unsigned char scaledPixel = (unsigned char)(pixel * scale);

                int i = (row * *width + x) * desiredChannels;
                for (int j = 0; j < desiredChannels; ++j)
                    data[i + j] = scaledPixel;
            }
        }
    }
    else
    {
        for (int y = 0; y < *height; ++y)
        {
            int row = invert ? (*height - 1 - y) : y;

            for (int x = 0; x < *width; ++x)
            {
                unsigned short pixel;
                file >> pixel;

                if (!file)
                {
                    std::cerr << "Failed to read pixel data from file" << std::endl;
                    delete[] data;
                    return nullptr;
                }

                unsigned char scaledPixel = (unsigned char)(pixel * scale);

                int i = (row * *width + x) * desiredChannels;
                for (int j = 0; j < desiredChannels; ++j)
                    data[i + j] = scaledPixel;
            }
        }
    }

    file.close();
    return data;
}