#include "JpgParser.h"
unsigned char* LoadJPG(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    FILE* infile;
    errno_t err = fopen_s(&infile, filename, "rb");
    if (err != 0) {
        printf("Error opening file %s\n", filename);
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    int rc = jpeg_read_header(&cinfo, TRUE);
    if (rc != 1) {
        printf("Error reading JPEG header\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *bpp = 4; // set bpp to 4 to indicate that the image data is in RGBA format

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    unsigned char* data = (unsigned char*)malloc(cinfo.output_width * cinfo.output_height * 4); // allocate memory for RGBA image data
    if (data == NULL) {
        printf("Error allocating memory for image data\n");
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    unsigned char* p = data;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        for (int i = 0; i < cinfo.output_width; i++) {
            p[0] = buffer[0][i * cinfo.output_components + 0];
            p[1] = buffer[0][i * cinfo.output_components + 1];
            p[2] = buffer[0][i * cinfo.output_components + 2];
            p[3] = 255; // set alpha channel to 255 (fully opaque)
            p += 4;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return data;
}