#include "OpenglSurface.h"
namespace primal::graphics::opengl
{
	namespace Dummy
	{
		HWND hwnd;
		HDC hdc;
		HGLRC hrc;
		WNDCLASSEX wcl;
	}
	bool opengl_surface::create_surface() {
		// Create the window
		Dummy::hwnd = (HWND)_window.handle();

		if (!Dummy::hwnd)
			return false;

		// Get the device context
		Dummy::hdc = GetDC(Dummy::hwnd);

		// Set up the pixel format
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,
			8,
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int pf = ChoosePixelFormat(Dummy::hdc, &pfd);
		if (!pf)
			return false;

		if (!SetPixelFormat(Dummy::hdc, pf, &pfd))
			return false;

		// Create the OpenGL rendering context
		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, 0,
			0
		};

		// Check if wglCreateContextAttribsARB is available
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		if (wglCreateContextAttribsARB)
		{
			Dummy::hrc = wglCreateContextAttribsARB(Dummy::hdc, 0, attribs);
		}
		else
		{
			Dummy::hrc = wglCreateContext(Dummy::hdc);
		}

		if (!Dummy::hrc)
			return false;

		// Make the rendering context current
		if (!wglMakeCurrent(Dummy::hdc, Dummy::hrc))
			return false;

		// Show the window
		ShowWindow(Dummy::hwnd, SW_SHOW);
		return true;
	}
	void opengl_surface::remove(){

	}
	void opengl_surface::resize(){

	}
	uint32_t opengl_surface::width(){
		RECT rect;
		if (GetWindowRect(Dummy::hwnd, &rect)) {
			return rect.right - rect.left;
		}
		return 0;
	}
	uint32_t opengl_surface::height(){
		RECT rect;
		if (GetWindowRect(Dummy::hwnd, &rect)) {
			return rect.top - rect.bottom;
		}
		return 0;
	}
	void opengl_surface::render_surface(){
	
	}
}