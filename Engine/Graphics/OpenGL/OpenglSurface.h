#pragma once
#include "OpenglCommonHeaders.h"
#include <GL/glew.h>
#include <wtypes.h>
#include "wglext.h"
namespace primal::graphics::opengl
{
	class opengl_surface
	{
	public:
		explicit opengl_surface(platform::window window) : _window{ window } {
			assert(_window.handle());
		}

		//DISABLE_COPY_AND_MOVE(opengl_surface);
		~opengl_surface() { /*release();*/ }
		bool create_surface();
		void remove();
		void resize();
		uint32_t width();
		uint32_t height();
		void render_surface();
	private:
		platform::window	_window{};
	};
}