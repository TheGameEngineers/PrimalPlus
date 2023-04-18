#include "WebpParser.h"
bool invertible = true;
void SetInversionible(bool value) {
    invertible = value;
}
unsigned char* LoadWebP(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    // Open the file
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0) {
        std::cerr << "Error: Failed to open file " << filename << std::endl;
        return nullptr;
    }

    // Get the size of the file
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // Read the file into memory
    uint8_t* fileData = new uint8_t[fileSize];
    size_t bytesRead = fread(fileData, sizeof(uint8_t), fileSize, file);
    if (bytesRead != fileSize) {
        std::cerr << "Error: Failed to read file " << filename << std::endl;
        delete[] fileData;
        fclose(file);
        return nullptr;
    }
    fclose(file);

    // Decode the WebP image data into RGBA format
    uint8_t* decodedData = WebPDecodeRGBA(fileData, fileSize, width, height);
    if (decodedData == nullptr) {
        std::cerr << "Error: Failed to decode WebP image" << std::endl;
        delete[] fileData;
        return nullptr;
    }

    // Free the memory used by the file data
    delete[] fileData;

    // Vertically invert the image if the inversion global variable is set to true
    if (invertible) {
        size_t rowSize = (*width) * desiredChannels;
        uint8_t* tempRow = new uint8_t[rowSize];
        for (size_t y = 0; y < (*height) / 2; y++) {
            uint8_t* row1 = decodedData + y * rowSize;
            uint8_t* row2 = decodedData + ((*height) - 1 - y) * rowSize;
            std::copy(row1, row1 + rowSize, tempRow);
            std::copy(row2, row2 + rowSize, row1);
            std::copy(tempRow, tempRow + rowSize, row2);
        }
        delete[] tempRow;
    }

    return decodedData;
}