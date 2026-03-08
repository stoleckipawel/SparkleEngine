// ============================================================================
// TextureLoader.h
// ----------------------------------------------------------------------------
// Loads image files from disk using Windows Imaging Component (WIC).
//
// USAGE:
//   TextureLoader loader("textures/diffuse.png");
//   const auto& data = loader.GetData();
//   // data.data, data.width, data.height, data.dxgiPixelFormat, etc.
//
// SUPPORTED FORMATS:
//   - Common image formats (PNG, JPEG, BMP, etc.) via WIC
//   - Limited DXGI format conversion (expand s_lookupTable as needed)
//
// DESIGN:
//   - Returns raw pixel data as uint8_t vector for unambiguous byte storage
//   - Data struct includes dimensions, stride, and DXGI format
//   - Conversion from WIC to DXGI format via lookup table
//
// NOTES:
//   - Throws on unsupported formats or load failures
//   - Pixel data is in CPU memory; caller uploads to GPU
// ============================================================================

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wincodec.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class AssetSystem;

class TextureLoader
{
  public:
	// Structure holding loaded image data and metadata.
	struct Data
	{
		std::vector<uint8_t> data;  // Raw image pixel data (bytes)
		uint32_t width = 1;         // Image width in pixels
		uint32_t height = 1;        // Image height in pixels
		uint32_t bitsPerPixel = 1;  // Bits per pixel
		uint32_t channelCount = 1;  // Number of color channels
		uint32_t stride = 1;        // Row pitch in bytes
		uint32_t slicePitch = 1;    // Total image size in bytes

		GUID wicPixelFormat = {};
		DXGI_FORMAT dxgiPixelFormat = DXGI_FORMAT_UNKNOWN;
	};

	explicit TextureLoader(const AssetSystem& assetSystem, const std::filesystem::path& fileName);

	const Data& GetData() const noexcept { return m_data; }

  private:
	// -------------------------------------------------------------------------
	// Loading Helpers
	// -------------------------------------------------------------------------

	ComPtr<IWICBitmapFrameDecode> DecodeImageFile(IWICImagingFactory* wicFactory, const std::filesystem::path& resolvedPath);
	void QueryPixelFormat(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame);
	void MapToDxgiFormat(const std::filesystem::path& resolvedPath);
	void CalculateBufferLayout();
	void CopyPixelData(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame);

	// -------------------------------------------------------------------------
	// State
	// -------------------------------------------------------------------------

	Data m_data;

	struct GUID_to_DXGI
	{
		GUID wic;
		DXGI_FORMAT dxgiFormat;
	};

	// Supported pixel format lookup table (defined in .cpp).
	static const std::vector<GUID_to_DXGI> s_lookupTable;

	bool m_requiresFormatConversion = false;
	GUID m_sourceWicPixelFormat = {};
	GUID m_targetWicPixelFormat = GUID_WICPixelFormat32bppRGBA;
};
