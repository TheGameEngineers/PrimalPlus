#pragma once
#include "OpenglCommonHeaders.h"
namespace primal::graphics::opengl
{
	struct opengl_frame_info
	{
		u32 surface_width{};
		u32 surface_height{};
	};
}
namespace primal::graphics::opengl::core
{
	bool initialize();
	void shutdown();
	surface create_surface(platform::window window);
	void remove_surface(surface_id id);
	void resize_surface(surface_id id, u32, u32);
	u32 surface_width(surface_id id);
	u32 surface_height(surface_id id);
	void render_surface(surface_id id);
}