#include "Importer.h"
#include <map>

enum FileType {
    BMP,
    GIF,
    HEIF,
    JPG,
    PBM,
    PGM,
    PNG,
    PNM,
    PPM,
    SVG,
    TGA,
    TIF,
    WEBP
};

std::map<std::string, FileType> fileTypes = {
    {"bmp", BMP},
    {"gif", GIF},
    {"heif", HEIF},
    {"jpg", JPG},
    {"jpeg", JPG},
    {"pbm", PBM},
    {"pgm", PGM},
    {"png", PNG},
    {"pnm", PNM},
    {"ppm", PPM},
    {"svg", SVG},
    {"tga", TGA},
    {"tif", TIF},
    {"tiff", TIF},
    {"webp", WEBP}
};

unsigned char* Load(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    std::string file(filename);
    std::string ext = file.substr(file.find_last_of(".") + 1);

    // Convert the file extension to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Check if the file extension is in the map
    if (fileTypes.count(ext) == 0) {
        return nullptr;
    }

    FileType type = fileTypes[ext];

    switch (type) {
    case BMP:
        return LoadBMP(filename, width, height, bpp, desiredChannels);
    case GIF:
        return LoadGIF(filename, width, height, bpp, desiredChannels);
    case HEIF:
        return LoadHEIF(filename, width, height, bpp, desiredChannels);
    case JPG:
        return LoadJPG(filename, width, height, bpp, desiredChannels);
    case PBM:
        return LoadPBM(filename, width, height, bpp, desiredChannels);
    case PGM:
        return LoadPGM(filename, width, height, bpp, desiredChannels);
    case PNG:
        return LoadPNG(filename, width, height, bpp, desiredChannels);
    case PNM:
        return LoadPNM(filename, width, height, bpp, desiredChannels);
    case PPM:
        return LoadPPM(filename, width, height, bpp, desiredChannels);
    case SVG:
        return LoadSVG(filename, width, height, bpp, desiredChannels);
    case TGA:
        return LoadTGA(filename, width, height, bpp, desiredChannels);
    case TIF:
        return LoadTIF(filename, width, height, bpp, desiredChannels);
    case WEBP:
        return LoadWebP(filename, width, height, bpp, desiredChannels);
    default:
        return nullptr;
    }
}