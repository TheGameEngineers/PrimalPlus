#include "OpenglCommonHeaders.h"
#include "OpenglInterface.h"
#include "OpenglCore.h"
namespace primal::graphics::opengl
{
	void
		get_platform_interface(platform_interface& pi) {
		pi.initialize = core::initialize;
		pi.shutdown = core::shutdown;
		pi.surface.create = core::create_surface;
		pi.surface.remove = core::remove_surface;
		pi.surface.resize = core::resize_surface;
		pi.surface.width = core::surface_width;
		pi.surface.height = core::surface_height;
		pi.surface.render = core::render_surface;
		pi.platform = graphics_platform::opengl;
	}
}