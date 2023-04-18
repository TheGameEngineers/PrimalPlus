#include "HeifParser.h"
unsigned char* LoadHEIF(const char* filename, int* width, int* height, int* bpp, int desiredChannels) {
    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        std::cerr << "Error allocating HEIF context" << std::endl;
        return nullptr;
    }

    heif_error error = heif_context_read_from_file(ctx, filename, nullptr);
    if (error.code != heif_error_Ok) {
        std::cerr << "Error reading HEIF file: " << error.message << std::endl;
        heif_context_free(ctx);
        return nullptr;
    }

    heif_image_handle* handle;
    error = heif_context_get_primary_image_handle(ctx, &handle);
    if (error.code != heif_error_Ok) {
        std::cerr << "Error getting primary image handle: " << error.message << std::endl;
        heif_context_free(ctx);
        return nullptr;
    }

    *width = heif_image_handle_get_width(handle);
    *height = heif_image_handle_get_height(handle);
    *bpp = desiredChannels;

    heif_image* img;
    error = heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);
    if (error.code != heif_error_Ok) {
        std::cerr << "Error decoding HEIF image: " << error.message << std::endl;
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return nullptr;
    }

    int stride;
    const uint8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

    std::vector<unsigned char> imageData(*width * *height * desiredChannels);
    std::copy(data, data + imageData.size(), imageData.data());

    heif_image_release(img);
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    unsigned char* result = new unsigned char[imageData.size()];
    std::copy(imageData.begin(), imageData.end(), result);
    return result;
}