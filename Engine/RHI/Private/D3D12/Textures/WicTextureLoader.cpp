#include "PCH.h"
#include "D3D12/Textures/WicTextureLoader.h"
#include "Core/Public/FileSystemUtils.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

static const auto g_wicTextureLoaderLogger = Logging::GetOrCreateLogger("RHI.Textures");

bool WicTextureLoader::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension != L".dds";
}

TextureLoadResult WicTextureLoader::Load(const std::filesystem::path& fileName) const
{
	WicTextureLoader loader(fileName);
	return std::move(loader.m_result);
}

const std::vector<WicTextureLoader::GuidToDxgi> WicTextureLoader::s_lookupTable = {
    {GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM},
    {GUID_WICPixelFormat32bppBGRA, DXGI_FORMAT_B8G8R8A8_UNORM}};

const std::vector<WicTextureLoader::GuidToWic> WicTextureLoader::s_convertTable = {
    {GUID_WICPixelFormat24bppBGR, GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM},
    {GUID_WICPixelFormat24bppRGB, GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM},
    {GUID_WICPixelFormat8bppGray, GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM},
    {GUID_WICPixelFormat32bppPBGRA, GUID_WICPixelFormat32bppBGRA, DXGI_FORMAT_B8G8R8A8_UNORM},
    {GUID_WICPixelFormat32bppPRGBA, GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM}};

WicTextureLoader::WicTextureLoader(const std::filesystem::path& fileName)
{
	const auto resolvedPath = Filesystem::ResolveAssetPathValidated(fileName, AssetType::Texture);

	ComPtr<IWICImagingFactory> wicFactory;
	CHECK(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicFactory.ReleaseAndGetAddressOf())));

	ComPtr<IWICBitmapFrameDecode> wicFrame = DecodeImageFile(wicFactory.Get(), resolvedPath);
	QueryPixelFormat(wicFrame.Get());
	MapToDxgiFormat(resolvedPath);
	CalculateBufferLayout();
	CopyPixelData(wicFactory.Get(), wicFrame.Get());
}

ComPtr<IWICBitmapFrameDecode> WicTextureLoader::DecodeImageFile(IWICImagingFactory* wicFactory, const std::filesystem::path& resolvedPath)
{
	ComPtr<IWICStream> wicFileStream;
	CHECK(wicFactory->CreateStream(wicFileStream.ReleaseAndGetAddressOf()));
	CHECK(wicFileStream->InitializeFromFilename(resolvedPath.wstring().c_str(), GENERIC_READ));

	ComPtr<IWICBitmapDecoder> wicDecoder;
	CHECK(wicFactory
	          ->CreateDecoderFromStream(wicFileStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, wicDecoder.ReleaseAndGetAddressOf()));

	ComPtr<IWICBitmapFrameDecode> wicFrame;
	CHECK(wicDecoder->GetFrame(0, wicFrame.ReleaseAndGetAddressOf()));

	CHECK(wicFrame->GetSize(&m_result.width, &m_result.height));

	return wicFrame;
}

void WicTextureLoader::QueryPixelFormat(IWICBitmapFrameDecode* wicFrame)
{
	CHECK(wicFrame->GetPixelFormat(&m_sourceWicPixelFormat));
}

void WicTextureLoader::MapToDxgiFormat(const std::filesystem::path& resolvedPath)
{
	m_requiresFormatConversion = false;
	m_targetWicPixelFormat = GUID_WICPixelFormat32bppRGBA;

	auto findIt = std::find_if(
	    s_lookupTable.begin(),
	    s_lookupTable.end(),
	    [&](const GuidToDxgi& entry)
	    {
		    return std::memcmp(&entry.wic, &m_sourceWicPixelFormat, sizeof(GUID)) == 0;
	    });

	if (findIt != s_lookupTable.end())
	{
		m_result.dxgiFormat = findIt->dxgiFormat;
		return;
	}

	auto convertIt = std::find_if(
	    s_convertTable.begin(),
	    s_convertTable.end(),
	    [&](const GuidToWic& entry)
	    {
		    return std::memcmp(&entry.sourceWic, &m_sourceWicPixelFormat, sizeof(GUID)) == 0;
	    });

	if (convertIt != s_convertTable.end())
	{
		m_requiresFormatConversion = true;
		m_targetWicPixelFormat = convertIt->targetWic;
		m_result.dxgiFormat = convertIt->dxgiFormat;
		return;
	}

	m_requiresFormatConversion = true;
	m_result.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	SPDLOG_LOGGER_WARN(
	    g_wicTextureLoaderLogger,
	    "WicTextureLoader: converting unsupported pixel format to 32bpp RGBA for file: {}",
	    resolvedPath.string());
}

void WicTextureLoader::CalculateBufferLayout()
{
	constexpr uint64_t bytesPerPixel = 4;
	const uint64_t stride64 = bytesPerPixel * static_cast<uint64_t>(m_result.width);
	const uint64_t slicePitch64 = stride64 * static_cast<uint64_t>(m_result.height);

	if (stride64 > (std::numeric_limits<uint32_t>::max)() || slicePitch64 > (std::numeric_limits<size_t>::max)())
	{
		Diagnostics::Fail(g_wicTextureLoaderLogger, __FILE__, __LINE__, "Texture too large or stride overflow");
	}

	TextureMipLevelData baseMip;
	baseMip.width = m_result.width;
	baseMip.height = m_result.height;
	baseMip.rowPitch = static_cast<uint32_t>(stride64);
	baseMip.slicePitch = static_cast<uint32_t>(slicePitch64);
	baseMip.data.resize(static_cast<size_t>(baseMip.slicePitch));
	m_result.arraySize = 1;
	m_result.dimension = TextureResourceDimension::Texture2D;
	m_result.arraySlices.clear();
	m_result.arraySlices.resize(1);
	m_result.arraySlices.front().mipLevels.push_back(std::move(baseMip));
}

void WicTextureLoader::CopyPixelData(IWICImagingFactory* wicFactory, IWICBitmapFrameDecode* wicFrame)
{
	WICRect copyRect = {0, 0, static_cast<INT>(m_result.width), static_cast<INT>(m_result.height)};
	TextureMipLevelData& baseMip = m_result.arraySlices.front().mipLevels.front();

	if (!m_requiresFormatConversion)
	{
		CHECK(wicFrame->CopyPixels(&copyRect, baseMip.rowPitch, baseMip.slicePitch, reinterpret_cast<BYTE*>(baseMip.data.data())));
		return;
	}

	ComPtr<IWICFormatConverter> formatConverter;
	CHECK(wicFactory->CreateFormatConverter(formatConverter.ReleaseAndGetAddressOf()));

	BOOL canConvert = FALSE;
	CHECK(formatConverter->CanConvert(m_sourceWicPixelFormat, m_targetWicPixelFormat, &canConvert));
	if (!canConvert)
	{
		Diagnostics::Fail(
		    g_wicTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    "WicTextureLoader: WIC cannot convert source pixel format to 32bpp RGBA.");
	}

	CHECK(
	    formatConverter->Initialize(wicFrame, m_targetWicPixelFormat, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom));
	CHECK(formatConverter->CopyPixels(&copyRect, baseMip.rowPitch, baseMip.slicePitch, reinterpret_cast<BYTE*>(baseMip.data.data())));
}
