#pragma once
#include "OpenglCommonHeaders.h"
namespace primal::graphics::opengl::index {
	class IndexBuffer {
	private:
		unsigned int m_RendererID;
		unsigned int m_Count;
	public:
		IndexBuffer(const unsigned int* data, unsigned int count);
		~IndexBuffer();
		void Bind() const;
		void Unbind() const;
		inline unsigned int GetCount() const { return m_Count; }
	};
}