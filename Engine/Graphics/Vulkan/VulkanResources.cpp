// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "VulkanResources.h"
#include "VulkanCore.h"

namespace primal::graphics::vulkan
{
namespace
{

} // anonymous namespace

bool
create_image(VkDevice device, VkImageType type, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags, bool create_view, VkImageAspectFlags view_aspect_flags, vulkan_image& image)
{
    VkResult result{ VK_SUCCESS };
    image.width = width;
    image.height = height;

    // Create image
    {
        VkImageCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        info.imageType = VK_IMAGE_TYPE_2D;					// TODO: This should be configurable
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;								// TODO: should be configurable and supported
        info.mipLevels = 1;									// TODO: should be configurable, and need to support mip maps
        info.arrayLayers = 1;								// TODO: should be configurable and offer image layer support
        info.format = format;
        info.tiling = tiling;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage = usage;
        info.samples = VK_SAMPLE_COUNT_1_BIT;				// TODO: make configurable
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		// TODO: make configurable

        VkCall(result = vkCreateImage(device, &info, nullptr, &image.image), "Failed to create image...");
        if (result != VK_SUCCESS) return false;
    }

    // Get memory requirements for image
    VkMemoryRequirements memory_reqs;
    vkGetImageMemoryRequirements(device, image.image, &memory_reqs);

    s32 index{ core::find_memory_index(memory_reqs.memoryTypeBits, memory_flags) };
    if (index == -1)
    {
        ERROR_MSSG("The required memory type was not found...");
    }

    // Allocate memory for image
    {
        VkMemoryAllocateInfo info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        info.allocationSize = memory_reqs.size;
        info.memoryTypeIndex = index;

        VkCall(result = vkAllocateMemory(device, &info, nullptr, &image.memory), "Failed to allocate memory for image...");
        if (result != VK_SUCCESS) return false;
    }

    // TODO: make memory offset configurable, for use in things like image pooling.
    VkCall(result = vkBindImageMemory(device, image.image, image.memory, 0), "Failed to bind image memory...");
    if (result != VK_SUCCESS) return false;

    if (create_view)
    {
        image.view = nullptr;
        if (!create_image_view(device, format, &image, view_aspect_flags)) return false;
    }

    return true;
}

bool
create_image_view(VkDevice device, VkFormat format, vulkan_image* image, VkImageAspectFlags view_aspect_flags)
{
    VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    info.image = image->image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;								// TODO: This should be configurable
    info.format = format;
    info.subresourceRange.aspectMask = view_aspect_flags;
    info.subresourceRange.baseMipLevel = 0;								// TODO: This should be configurable
    info.subresourceRange.levelCount = 1;								// TODO: This should be configurable
    info.subresourceRange.baseArrayLayer = 0;							// TODO: This should be configurable
    info.subresourceRange.layerCount = 1;								// TODO: This should be configurable

    VkResult result{ VK_SUCCESS };
    VkCall(result = vkCreateImageView(device, &info, nullptr, &image->view), "Failed to create image view...");
    if (result != VK_SUCCESS) return false;

    return true;
}

void
destroy_image(VkDevice device, vulkan_image* image)
{
    if (image->view)
    {
        vkDestroyImageView(device, image->view, nullptr);
        image->view = nullptr;
    }
    if (image->memory)
    {
        vkFreeMemory(device, image->memory, nullptr);
        image->memory = nullptr;
    }
    if (image->image)
    {
        vkDestroyImage(device, image->image, nullptr);
        image->image = nullptr;
    }
}

bool
create_framebuffer(VkDevice device, vulkan_renderpass& renderpass, u32 width, u32 height, u32 attach_count, utl::vector<VkImageView> attachments, vulkan_framebuffer& framebuffer)
{
    framebuffer.attachments.resize(attach_count);
    for (u32 i{ 0 }; i < attach_count; ++i)
    {
        framebuffer.attachments[i] = attachments[i];
    }
    framebuffer.renderpass = &renderpass;
    framebuffer.attach_count = attach_count;

    VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    info.renderPass = renderpass.render_pass;
    info.attachmentCount = attach_count;
    info.pAttachments = framebuffer.attachments.data();
    info.width = width;
    info.height = height;
    info.layers = 1;

    VkResult result{ VK_SUCCESS };
    VkCall(result = vkCreateFramebuffer(device, &info, nullptr, &framebuffer.framebuffer), "Failed to create framebuffer...");
    if (result != VK_SUCCESS) return false;

    MESSAGE("Created frambuffer");

    return true;
}

void
destroy_framebuffer(VkDevice device, vulkan_framebuffer& framebuffer)
{
    vkDestroyFramebuffer(device, framebuffer.framebuffer, nullptr);
    if (framebuffer.attachments.data())
        framebuffer.attachments.clear();

    framebuffer.framebuffer = nullptr;
    framebuffer.renderpass = nullptr;
    framebuffer.attach_count = 0;

    MESSAGE("Destroyed framebuffer");
}
}