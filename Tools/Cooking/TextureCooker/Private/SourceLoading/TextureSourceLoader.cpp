#include "PCH.h"

#include "SourceLoading/TextureSourceLoader.h"

#include "SourceLoading/DdsTextureSourceLoader.h"
#include "SourceLoading/ExrTextureSourceLoader.h"
#include "SourceLoading/HdrTextureSourceLoader.h"
#include "SourceLoading/RasterTextureSourceLoader.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"

#include <array>
#include <format>

TextureLoadResult TextureSourceLoader::Load(const std::filesystem::path& sourcePath)
{
	static const DdsTextureSourceLoader ddsTextureSourceLoader;
	static const ExrTextureSourceLoader exrTextureSourceLoader;
	static const HdrTextureSourceLoader hdrTextureSourceLoader;
	static const RasterTextureSourceLoader rasterTextureSourceLoader;
	static const std::array<const TextureSourceLoaderBackend*, 4> textureSourceLoaderBackends =
	    {&ddsTextureSourceLoader, &exrTextureSourceLoader, &hdrTextureSourceLoader, &rasterTextureSourceLoader};

	const TextureSourceFormat format = ResolveFormat(sourcePath);
	for (const TextureSourceLoaderBackend* textureSourceLoaderBackend : textureSourceLoaderBackends)
	{
		if (textureSourceLoaderBackend->SupportsFormat(format))
		{
			return textureSourceLoaderBackend->Load(sourcePath);
		}
	}

	throw Diagnostics::Error(std::format("Unsupported source texture format for '{}'.", sourcePath.string()));
}

TextureSourceFormat TextureSourceLoader::ResolveFormat(const std::filesystem::path& sourcePath) noexcept
{
	const std::wstring extension = Paths::GetLowercaseExtension(sourcePath);
	if (extension == L".dds")
	{
		return TextureSourceFormat::Dds;
	}

	if (extension == L".exr")
	{
		return TextureSourceFormat::Exr;
	}

	if (extension == L".hdr" || extension == L".hdri")
	{
		return TextureSourceFormat::RadianceHdr;
	}

	if (extension == L".png" || extension == L".jpg" || extension == L".jpeg" || extension == L".bmp" || extension == L".tga"
	    || extension == L".gif" || extension == L".psd" || extension == L".pic" || extension == L".pnm" || extension == L".ppm"
	    || extension == L".pgm")
	{
		return TextureSourceFormat::StandardRaster;
	}

	return TextureSourceFormat::Unknown;
}
