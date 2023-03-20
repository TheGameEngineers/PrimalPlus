// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once

#include "VulkanCommonHeaders.h"

namespace primal::graphics::vulkan::core {
	
	bool initialize();
	void shutdown();

	bool create_device(VkSurfaceKHR surface);
	bool create_graphics_command(u32 swapchain_framebuffer_size);
	bool detect_depth_format(VkPhysicalDevice physical_device);
	s32 find_memory_index(u32 type, u32 flags);

	u32 graphics_family_queue_index();
	u32 presentation_family_queue_index();
	VkFormat depth_format();
	VkPhysicalDevice physical_device();
	VkDevice logical_device();
	VkInstance get_instance();

	surface create_surface(platform::window window);
	void remove_surface(surface_id id);
	void resize_surface(surface_id id, u32 width, u32 height);
	u32 surface_width(surface_id id);
	u32 surface_height(surface_id id);
	void render_surface(surface_id id, frame_info info);
}