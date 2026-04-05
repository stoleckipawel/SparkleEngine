#pragma once

#include "D3D12/Textures/TextureLoaderBackend.h"

#include <cstdint>
#include <filesystem>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wincodec.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class WicTextureLoader final : public TextureLoaderBackend
{
  public:
	WicTextureLoader() = default;

	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& fileName) const override;

  private:
	explicit WicTextureLoader(const std::filesystem::path& fileName);

	ComPtr<IWICBitmapFrameDecode> DecodeImageFile(IWICImagingFactory* wicFactory, const std::filesystem::path& resolvedPath);
	void QueryPixelFormat(IWICBitmapFrameDecode* wicFrame);
	void MapToDxgiFormat(const std::filesystem::path& resolvedPath);
	void CalculateBufferLayout();
	void CopyPixelData(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame);

	TextureLoadResult m_result;

	struct GuidToDxgi
	{
		GUID wic;
		DXGI_FORMAT dxgiFormat;
	};

	struct GuidToWic
	{
		GUID sourceWic;
		GUID targetWic;
		DXGI_FORMAT dxgiFormat;
	};

	static const std::vector<GuidToDxgi> s_lookupTable;
	static const std::vector<GuidToWic> s_convertTable;

	bool m_requiresFormatConversion = false;
	GUID m_sourceWicPixelFormat = {};
	GUID m_targetWicPixelFormat = GUID_WICPixelFormat32bppRGBA;
};