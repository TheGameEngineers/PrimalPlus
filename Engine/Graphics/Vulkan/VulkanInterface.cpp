// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "VulkanInterface.h"
#include "VulkanCore.h"
#include "Graphics/GraphicsPlatformInterface.h"
#include "CommonHeaders.h"

namespace primal::graphics::vulkan {
void
get_platform_interface(platform_interface& pi)
{
    pi.initialize = core::initialize;
    pi.shutdown = core::shutdown;

    pi.surface.create = core::create_surface;
    pi.surface.remove = core::remove_surface;
    pi.surface.resize = core::resize_surface;
    pi.surface.width = core::surface_width;
    pi.surface.height = core::surface_height;
    pi.surface.render = core::render_surface;

    // pi.light.create = light::create;
    // pi.light.remove = light::remove;
    // pi.light.set_paramter = light::set_paramter;
    // pi.light.get_paramter = light::get_paramter;

    // pi.camera.create = camera::create;
    // pi.camera.remove = camera::remove;
    // pi.camera.set_paramter = camera::set_paramter;
    // pi.camera.get_paramter = camera::get_paramter;

    // pi.resources.add_submesh = content::submesh::add;
    // pi.resources.remove_submesh = content::submesh::remove;
    // pi.resources.add_material = content::material::add;
    // pi.resources.remove_material = content::material::remove;
    // pi.resources.add_render_item = content::render_item::add;
    // pi.resources.remove_render_item = content::render_item::remove;

    pi.platform = graphics_platform::vulkan_1;
}

}