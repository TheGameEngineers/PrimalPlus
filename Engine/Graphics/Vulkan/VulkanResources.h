// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "VulkanCommonHeaders.h"

namespace primal::graphics::vulkan {

struct image_init_info
{
    VkDevice                device;
    VkImageType             image_type;
    u32                     width;
    u32                     height;
    VkFormat                format;
    VkImageTiling           tiling;
    VkImageUsageFlags       usage_flags;
    VkMemoryPropertyFlags   memory_flags;
    bool                    create_view;
    VkImageAspectFlags      view_aspect_flags;
};

bool create_image(const image_init_info* const init_info, vulkan_image& image);
bool create_image_view(VkDevice device, VkFormat format, vulkan_image* image, VkImageAspectFlags view_aspect_flags);
void destroy_image(VkDevice device, vulkan_image* image);

bool create_framebuffer(VkDevice device, vulkan_renderpass& renderpass, u32 width, u32 height, u32 attach_count, VkImageView* attachments, vulkan_framebuffer& framebuffer);
void destroy_framebuffer(VkDevice device, vulkan_framebuffer& framebuffer);

}