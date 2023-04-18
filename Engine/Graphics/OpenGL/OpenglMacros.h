#pragma once
#include <iostream>
#include <GL/glew.h>
#define ASSERT(x) if (!(x)) __debugbreak();
#ifdef NDEBUG
#define GLCall(x) x;
#else
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#endif
void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);