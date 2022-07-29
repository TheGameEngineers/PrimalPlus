#pragma once
#include "VulkanCommonHeaders.h"

namespace primal::graphics::vulkan
{
bool create_image(VkDevice device, VkImageType type, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags, bool create_view, VkImageAspectFlags view_aspect_flags, vulkan_image& image);
bool create_image_view(VkDevice device, VkFormat format, vulkan_image* image, VkImageAspectFlags view_aspect_flags);
void destroy_image(VkDevice device, vulkan_image* image);

bool create_framebuffer(VkDevice device, vulkan_renderpass& renderpass, u32 width, u32 height, u32 attach_count, utl::vector<VkImageView> attachments, vulkan_framebuffer& framebuffer);
void destroy_framebuffer(VkDevice device, vulkan_framebuffer& framebuffer);
}