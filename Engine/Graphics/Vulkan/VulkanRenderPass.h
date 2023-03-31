#pragma once
#include "VulkanCommonHeaders.h"

namespace primal::graphics::vulkan::renderpass {
	
vulkan_renderpass create_renderpass(VkDevice device, VkFormat swapchain_image_format, VkFormat depth_format, math::u32v4 render_area, math::v4 clear_color, f32 depth, u32 stencil);
void destroy_renderpass(VkDevice device, vulkan_renderpass& renderpass);
void begin_renderpass(VkCommandBuffer cmd_buffer, vulkan_cmd_buffer::state& state, vulkan_renderpass& renderpass, VkFramebuffer frame_buffer);
void end_renderpass(VkCommandBuffer cmd_buffer, vulkan_cmd_buffer::state& state, vulkan_renderpass& renderpass);

}