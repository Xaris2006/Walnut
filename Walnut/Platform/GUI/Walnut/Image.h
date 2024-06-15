#pragma once

#include <string>

namespace Walnut {

	enum class ImageFormat
	{
		None = 0,
		RGBA,
		RGB,
		RGBA32F
	};

	class Image
	{
	public:
		Image(std::string_view path);
		Image(uint32_t width, uint32_t height, ImageFormat format, const void* data = nullptr);
		~Image();

		void SetData(const void* data);

		uint32_t GetRendererID() const { return m_RendererID; }


		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		static void* Decode(const void* data, uint64_t length, uint32_t& outWidth, uint32_t& outHeight);
	
	private:
		uint32_t m_Width = 0, m_Height = 0;

		ImageFormat m_DataFormat;

		size_t m_AlignedSize = 0;

		uint32_t m_RendererID = 0;

		std::string m_Filepath;
	};

}



