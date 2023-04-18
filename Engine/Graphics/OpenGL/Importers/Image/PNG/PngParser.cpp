#include "PngParser.h"
bool inversion = true;
void ToggleInversion(bool value)
{
    inversion = value;
}
unsigned char* LoadPNG(const char* filename, int* width, int* height, int* bpp, int desiredChannels)
{
    FILE* fp;
    errno_t err = fopen_s(&fp, filename, "rb");
    if (err != 0) abort();

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    *bpp = desiredChannels;

    unsigned char* image = (unsigned char*)malloc((*width) * (*height) * (*bpp));
    unsigned char** row_pointers = (unsigned char**)malloc(sizeof(unsigned char*) * (*height));
    for (int y = 0; y < (*height); y++) {
        row_pointers[y] = image + y * (*width) * (*bpp);
    }

    png_read_image(png, row_pointers);
    if (inversion){    
        // Flip the image vertically
        for (int y = 0; y < (*height) / 2; y++) {
            unsigned char* row1 = row_pointers[y];
            unsigned char* row2 = row_pointers[(*height) - 1 - y];
            for (int x = 0; x < (*width) * (*bpp); x++) {
                unsigned char temp = row1[x];
                row1[x] = row2[x];
                row2[x] = temp;
            }
        }
    }
    fclose(fp);

    return image;
}