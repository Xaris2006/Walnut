#include "Image.h"

#include "imgui.h"

#include "ApplicationGUI.h"

#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Walnut {

	namespace Utils {

		static uint32_t BytesPerPixel(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::RGBA:    return 4;
				case ImageFormat::RGBA32F: return 16;
			}
			return 0;
		}

		static GLenum HazelImageFormatToGLDataFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA: return GL_RGBA;
			case ImageFormat::RGBA32F: return GL_RGBA;
			}

			return 0;
		}

		static GLenum HazelImageFormatToGLInternalFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::RGBA: return GL_RGBA8;
			case ImageFormat::RGBA32F: return GL_RGBA16;
			}

			return 0;
		}

	}

	Image::Image(std::string_view path)
		: m_Filepath(path)
	{
		int width, height, channels;
		uint8_t* data = nullptr;

		if (stbi_is_hdr(m_Filepath.c_str()))
		{
			data = (uint8_t*)stbi_loadf(m_Filepath.c_str(), &width, &height, &channels, 4);
			m_DataFormat = ImageFormat::RGBA32F;
		}
		else
		{
			data = stbi_load(m_Filepath.c_str(), &width, &height, &channels, 4);
			m_DataFormat = ImageFormat::RGBA;
		}

		m_Width = width;
		m_Height = height;
		
		if(data)
			SetData(data);
		
		stbi_image_free(data);
	}

	Image::Image(uint32_t width, uint32_t height, ImageFormat format, const void* data)
		: m_Width(width), m_Height(height), m_DataFormat(format)
	{
		if (data)
			SetData(data);
	}

	Image::~Image()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void Image::SetData(const void* data)
	{
		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		glTexImage2D(GL_TEXTURE_2D, 0, Utils::HazelImageFormatToGLInternalFormat(m_DataFormat), m_Width, m_Height, 0, Utils::HazelImageFormatToGLDataFormat(m_DataFormat), GL_UNSIGNED_BYTE, data);
	}

	void* Image::Decode(const void* buffer, uint64_t length, uint32_t& outWidth, uint32_t& outHeight)
	{
		int width, height, channels;
		uint8_t* data = nullptr;
		uint64_t size = 0;

		data = stbi_load_from_memory((const stbi_uc*)buffer, length, &width, &height, &channels, 4);
		size = width * height * 4;

		outWidth = width;
		outHeight = height;

		return data;
	}

}
