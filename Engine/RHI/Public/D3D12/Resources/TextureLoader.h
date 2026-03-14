#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wincodec.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class TextureLoader
{
  public:
	struct Data
	{
		std::vector<uint8_t> data;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t bitsPerPixel = 1;
		uint32_t channelCount = 1;
		uint32_t stride = 1;
		uint32_t slicePitch = 1;

		GUID wicPixelFormat = {};
		DXGI_FORMAT dxgiPixelFormat = DXGI_FORMAT_UNKNOWN;
	};

	explicit TextureLoader(const std::filesystem::path& fileName);

	const Data& GetData() const noexcept { return m_data; }

  private:
	ComPtr<IWICBitmapFrameDecode> DecodeImageFile(IWICImagingFactory* wicFactory, const std::filesystem::path& resolvedPath);
	void QueryPixelFormat(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame);
	void MapToDxgiFormat(const std::filesystem::path& resolvedPath);
	void CalculateBufferLayout();
	void CopyPixelData(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame);

	Data m_data;

	struct GUID_to_DXGI
	{
		GUID wic;
		DXGI_FORMAT dxgiFormat;
	};

	struct GUID_to_WIC
	{
		GUID sourceWic;
		GUID targetWic;
		DXGI_FORMAT dxgiFormat;
	};

	static const std::vector<GUID_to_DXGI> s_lookupTable;
	static const std::vector<GUID_to_WIC> s_convertTable;

	bool m_requiresFormatConversion = false;
	GUID m_sourceWicPixelFormat = {};
	GUID m_targetWicPixelFormat = GUID_WICPixelFormat32bppRGBA;
};
