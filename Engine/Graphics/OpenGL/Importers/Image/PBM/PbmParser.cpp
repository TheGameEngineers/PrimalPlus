#include "PbmParser.h"
bool inverts = true;
void SetInversions(bool value) {
    inverts = value;
}
void RemoveCarriageReturns(std::string& str)
{
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}
unsigned char* LoadPBM(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    *bpp = 1;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;
    while (std::getline(file, line))
    {
        RemoveCarriageReturns(line);
        if (!line.empty() && line[0] != '#')
            break;
    }

    bool isBinary = (line == "P4");
    if (!isBinary && line != "P1")
    {
        std::cerr << "Invalid PBM file: " << filename << std::endl;
        return nullptr;
    }

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

    int size = *width * *height * desiredChannels;
    unsigned char* data = new unsigned char[size];

    if (isBinary)
    {
        int rowSize = (*width + 7) / 8;
        std::vector<char> buffer(rowSize);

        for (int y = 0; y < *height; ++y)
        {
            int row = inverts ? (*height - 1 - y) : y;

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
                unsigned char pixel = ((*rowData >> (7 - x % 8)) & 1) ? 0 : 255;

                if (x % 8 == 7)
                    ++rowData;

                int i = (row * *width + x) * desiredChannels;
                for (int j = 0; j < desiredChannels; ++j)
                    data[i + j] = pixel;
            }
        }
    }
    else
    {
        for (int y = 0; y < *height; ++y)
        {
            int row = inverts ? (*height - 1 - y) : y;

            for (int x = 0; x < *width; ++x)
            {
                char pixel;
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

                int i = (row * *width + x) * desiredChannels;
                for (int j = 0; j < desiredChannels; ++j)
                    data[i + j] = (pixel == '1') ? 0 : 255;
            }
        }
    }

    file.close();
    return data;
}