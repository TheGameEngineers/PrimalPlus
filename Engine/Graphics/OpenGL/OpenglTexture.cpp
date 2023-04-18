#include "OpenglTexture.h"
#include "Graphics/OpenGL/Importers/Common/Importer.h"
namespace primal::graphics::opengl::texture {
	Texture::Texture(const std::string& path) :m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0) {
		//stbi_set_flip_vertically_on_load(1);
		//m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);
		//m_LocalBuffer = LoadBMP(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);
		m_LocalBuffer = Load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);
		if (m_LocalBuffer == nullptr)
		{
			std::cerr << "Error loading file" << std::endl;
		}
		else
		{
			GLCall(glGenTextures(1, &m_RendererID));
			GLenum error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error generating texture: " << gluErrorString(error) << std::endl;
			}

			GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error binding texture: " << gluErrorString(error) << std::endl;
			}

			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error setting texture parameter: " << gluErrorString(error) << std::endl;
			}

			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error setting texture parameter: " << gluErrorString(error) << std::endl;
			}

			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error setting texture parameter: " << gluErrorString(error) << std::endl;
			}

			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error setting texture parameter: " << gluErrorString(error) << std::endl;
			}

			GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer));
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				std::cerr << "Error uploading texture data: " << gluErrorString(error) << std::endl;
			}
		}
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
		if (m_LocalBuffer)
			return;
		//stbi_image_free(m_LocalBuffer);
	}
	Texture::~Texture() {
		GLCall(glDeleteTextures(1, &m_RendererID));
	}
	void Texture::Bind(unsigned int slot) const {
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
	}
	void Texture::Unbind() const {
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}
}