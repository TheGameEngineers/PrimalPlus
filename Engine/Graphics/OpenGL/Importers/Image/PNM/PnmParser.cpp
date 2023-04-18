#include "PnmParser.h"
bool invertit = true;
void SetInversionit(bool value) {
    invertit = value;
}
void RemoveCarriageReturn(std::string& str)
{
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}
unsigned char* LoadPNM(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;
    while (std::getline(file, line))
    {
        RemoveCarriageReturn(line);
        if (!line.empty() && line[0] != '#')
            break;
    }

    int format = -1;
    if (line == "P1")
        format = 1;
    else if (line == "P2")
        format = 2;
    else if (line == "P3")
        format = 3;
    else if (line == "P4")
        format = 4;
    else if (line == "P5")
        format = 5;
    else if (line == "P6")
        format = 6;

    if (format == -1)
    {
        std::cerr << "Invalid PNM file: " << filename << std::endl;
        return nullptr;
    }

    bool isBinary = (format >= 4);

    while (std::getline(file, line))
    {
        if (!line.empty() && line[0] != '#')
            break;
    }

    std::stringstream ss(line);
    ss >> *width >> *height;

    if (*width <= 0 || *height <= 0)
    {
        std::cerr << "Invalid image dimensions: " << *width << "x" << *height << std::endl;
        return nullptr;
    }

    int channels = 0;
    switch (format)
    {
    case 1:
    case 4:
        channels = 1;
        *bpp = 1;
        break;

    case 2:
    case 5:
        channels = 1;
        while (std::getline(file, line))
        {
            if (!line.empty() && line[0] != '#')
                break;
        }
        ss.str(line);
        ss.clear();
        ss >> *bpp;
        break;

    case 3:
    case 6:
        channels = 3;
        while (std::getline(file, line))
        {
            if (!line.empty() && line[0] != '#')
                break;
        }
        ss.str(line);
        ss.clear();
        ss >> *bpp;
        break;

    default:
        std::cerr << "Invalid PNM format: " << format << std::endl;
        return nullptr;
    }

    if (*bpp <= 0 || *bpp > 65535)
    {
        std::cerr << "Invalid maximum pixel value: " << *bpp << std::endl;
        return nullptr;
    }

    int size = *width * *height * desiredChannels;
    unsigned char* data = new unsigned char[size];

    float scale = (*bpp == 1) ? 255.0f : (255.0f / *bpp);

    if (isBinary)
    {
        int rowSize = (*width) * channels * (*bpp <= 255 ? 1 : 2);
        std::vector<char> buffer(rowSize);

        for (int y = 0; y < *height; ++y)
        {
            int row = invertit ? (*height - 1 - y) : y;

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
                for (int c = 0; c < channels; ++c)
                {
                    unsigned short pixel =
                        (*bpp <= 255) ? (unsigned char)*rowData++ : *(unsigned short*)rowData++;
                    unsigned char scaledPixel =
                        (*bpp == 1) ? ((pixel == 0) ? 255 : 0) : ((unsigned char)(pixel * scale));

                    int i = (row * *width + x) * desiredChannels;
                    for (int j = 0; j < desiredChannels; ++j)
                        data[i + j] = scaledPixel;
                }
            }
        }
    }
    else
    {
        for (int y = 0; y < *height; ++y)
        {
            int row = invertit ? (*height - 1 - y) : y;

            for (int x = 0; x < *width; ++x)
            {
                for (int c = 0; c < channels; ++c)
                {
                    unsigned short pixel;
                    do
                    {
                        file >> pixel;
                        if (!file)
                        {
                            std::cerr << "Failed to read pixel data from file" << std::endl;
                            delete[] data;
                            return nullptr;
                        }
                    } while (pixel == '#');

                    unsigned char scaledPixel =
                        (*bpp == 1) ? ((pixel == 0) ? 255 : 0) : ((unsigned char)(pixel * scale));

                    int i = (row * *width + x) * desiredChannels;
                    for (int j = 0; j < desiredChannels; ++j)
                        data[i + j] = scaledPixel;
                }
            }
        }
    }

    file.close();
    return data;
}