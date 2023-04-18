#include "PpmParser.h"
bool flipped = false;
void ToggleFlipped(bool value)
{
    flipped = value;
}
unsigned char* LoadPPM_P6(std::ifstream& file, int* width, int* height, int* bpp, int desiredChannels, std::string line) {

    while (std::getline(file, line))
    {
        if (line[0] == '#') continue;
        std::istringstream iss(line);
        iss >> *width >> *height;
        break;
    }

    std::getline(file, line);
    *bpp = std::stoi(line);

    const int srcNumChannels = 3;
    const int dataSize = (*width) * (*height) * srcNumChannels;
    unsigned char* data = new unsigned char[dataSize];
    file.read(reinterpret_cast<char*>(data), dataSize);
    return data;
}
unsigned char* LoadPPM_P3(std::ifstream& file, int* width, int* height, int* bpp, int desiredChannels, std::string line)
{
    while (std::getline(file, line))
    {
        if (line[0] == '#') continue;
        std::istringstream iss(line);
        iss >> *width >> *height;
        break;
    }

    std::getline(file, line);
    *bpp = std::stoi(line);

    const int srcNumChannels = 3;
    const int dataSize = (*width) * (*height) * srcNumChannels;
    unsigned char* data = new unsigned char[dataSize];

    for (int i = 0; i < dataSize; ++i)
    {
        int value;
        file >> value;
        data[i] = static_cast<unsigned char>(value);
    }

    return data;
}
unsigned char* LoadPPM(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;
    std::getline(file, line);
    unsigned char* data = nullptr;
    if (line == "P6") {
        data = LoadPPM_P6(file, width, height, bpp, desiredChannels, line);
    }
    else if (line == "P3")
    {
        data = LoadPPM_P3(file, width, height, bpp, desiredChannels, line);
    }
    else
    {
        std::cerr << "Invalid PPM file format: " << line << std::endl;
        return nullptr;
    }
    const int srcNumChannels = 3;
    if (flipped) {
        // Flip the image vertically
        const int rowSize = (*width) * srcNumChannels;
        unsigned char* rowBuffer = new unsigned char[rowSize];
        for (int y = 0; y < (*height) / 2; ++y)
        {
            unsigned char* row1 = data + y * rowSize;
            unsigned char* row2 = data + ((*height) - 1 - y) * rowSize;
            memcpy(rowBuffer, row1, rowSize);
            memcpy(row1, row2, rowSize);
            memcpy(row2, rowBuffer, rowSize);
        }
        delete[] rowBuffer;
    }
    if (desiredChannels != srcNumChannels)
    {
        unsigned char* convertedData = ConvertImageFormat(data, *width, *height, srcNumChannels, desiredChannels);
        delete[] data;
        data = convertedData;
    }

    return data;
}
unsigned char* ConvertImageFormat(const unsigned char* data, int width, int height, int srcNumChannels, int dstNumChannels)
{
	// Calculate the size of the converted image data
	int imageSize = width * height * dstNumChannels;

	// Allocate memory for the converted image data
	unsigned char* convertedData = new unsigned char[imageSize];

	// Convert the image data
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			for (int c = 0; c < dstNumChannels; ++c)
			{
				if (c < srcNumChannels)
				{
					// Copy the channel data from the source image
					convertedData[(y * width + x) * dstNumChannels + c] = data[(y * width + x) * srcNumChannels + c];
				}
				else
				{
					// Set the additional channels to a default value (e.g. 255 for an alpha channel)
					convertedData[(y * width + x) * dstNumChannels + c] = 255;
				}
			}
		}
	}

	return convertedData;
}