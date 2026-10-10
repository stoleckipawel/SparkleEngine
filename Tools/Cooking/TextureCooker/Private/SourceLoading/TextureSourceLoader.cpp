#include "PCH.h"

#include "SourceLoading/TextureSourceLoader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"

#include <format>

TextureLoadResult LoadTextureSource(const std::filesystem::path& sourcePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(sourcePath);
	if (extension == L".dds")
	{
		return LoadDdsTextureSource(sourcePath);
	}
	if (extension == L".exr")
	{
		return LoadExrTextureSource(sourcePath);
	}
	if (extension == L".hdr" || extension == L".hdri")
	{
		return LoadHdrTextureSource(sourcePath);
	}
	if (extension == L".png" || extension == L".jpg" || extension == L".jpeg" || extension == L".bmp" || extension == L".tga" || extension == L".gif" || extension == L".psd" || extension == L".pic"
	    || extension == L".pnm" || extension == L".ppm" || extension == L".pgm")
	{
		return LoadRasterTextureSource(sourcePath);
	}
	throw Diagnostics::Error(std::format("Unsupported source texture format for '{}'.", sourcePath.string()));
}
