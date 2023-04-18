#pragma once
#include "OpenglCommonHeaders.h"
namespace primal::graphics::opengl::vertex {
	class VertexBuffer {
	private:
		unsigned int m_RendererID;
	public:
		VertexBuffer(const void* data, unsigned int size);
		~VertexBuffer();
		void Bind() const;
		void Unbind() const;
	};
}