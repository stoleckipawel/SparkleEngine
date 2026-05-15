#pragma once

#include "Textures/CookedTextureAsset.h"
#include "D3D12/Textures/TextureLoadResult.h"

#include "Core/Public/Files/BinarySpanReader.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class CookedTextureAssetLoader final
{
  public:
	bool SupportsExtension(std::wstring_view extension) const noexcept;
	TextureLoadResult Load(const std::filesystem::path& fileName) const;

  private:
	static bool ValidateHeader(
	    const CookedTextureAssetHeader& header,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage);
	static bool ValidateMipHeader(
	    const CookedTextureMipHeader& mipHeader,
	    std::uint32_t mipIndex,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage);
	static bool ReadMipHeaders(
	    Files::BinarySpanReader& reader,
	    std::uint32_t mipCount,
	    const std::filesystem::path& resolvedPath,
	    std::vector<CookedTextureMipHeader>& outMipHeaders,
	    std::string& outErrorMessage);
	static bool ReadMipPayloads(
	    Files::BinarySpanReader& reader,
	    const std::vector<CookedTextureMipHeader>& mipHeaders,
	    const std::filesystem::path& resolvedPath,
	    TextureLoadResult& outLoadResult,
	    std::string& outErrorMessage);
	static bool TryResolveFormatIntent(std::uint32_t storedValue, TextureFormatIntent& outFormatIntent) noexcept;
};