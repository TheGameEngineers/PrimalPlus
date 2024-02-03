#include "D3D11Interface.h"
#include "D3D11Core.h"
#include "D3D11Camera.h"
#include "D3D11Content.h"
#include "D3D11Light.h"
#include "Graphics/GraphicsPlatformInterface.h"

namespace primal::graphics::d3d11 {
#if PRIMAL_BUILD_D3D11
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

	pi.light.create_light_set = light::create_light_set;
	pi.light.remove_light_set = light::remove_light_set;
	pi.light.create = light::create;
	pi.light.remove = light::remove;
	pi.light.set_parameter = light::set_parameter;
	pi.light.get_parameter = light::get_parameter;

	pi.camera.create = camera::create;
	pi.camera.remove = camera::remove;
	pi.camera.set_parameter = camera::set_parameter;
	pi.camera.get_parameter = camera::get_parameter;

	pi.resources.add_submesh = content::submesh::add;
	pi.resources.remove_submesh = content::submesh::remove;
	pi.resources.add_material = content::material::add;
	pi.resources.remove_material = content::material::remove;
	pi.resources.add_render_item = content::render_item::add;
	pi.resources.remove_render_item = content::render_item::remove;

	pi.platform = graphics_platform::direct3d11;
}
#else
void
get_platform_interface(platform_interface& pi)
{}
#endif
}