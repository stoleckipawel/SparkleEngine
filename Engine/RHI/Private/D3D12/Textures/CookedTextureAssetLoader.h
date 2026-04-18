#pragma once

#include "D3D12/Textures/CookedTextureAsset.h"
#include "D3D12/Textures/TextureLoaderBackend.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class CookedTextureAssetLoader final : public TextureLoaderBackend
{
  public:
	bool SupportsExtension(std::wstring_view extension) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& fileName) const override;

  private:
	static bool ReadBytes(
	    const std::vector<std::uint8_t>& fileBytes,
	    std::size_t& byteOffset,
	    void* destination,
	    std::size_t byteCount,
	    std::string& outErrorMessage);
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
	    const std::vector<std::uint8_t>& fileBytes,
	    std::size_t& byteOffset,
	    std::uint32_t mipCount,
	    const std::filesystem::path& resolvedPath,
	    std::vector<CookedTextureMipHeader>& outMipHeaders,
	    std::string& outErrorMessage);
	static bool ReadMipPayloads(
	    const std::vector<std::uint8_t>& fileBytes,
	    std::size_t& byteOffset,
	    const std::vector<CookedTextureMipHeader>& mipHeaders,
	    const std::filesystem::path& resolvedPath,
	    TextureLoadResult& outLoadResult,
	    std::string& outErrorMessage);
	static bool TryResolveFormatIntent(std::uint32_t storedValue, TextureFormatIntent& outFormatIntent) noexcept;
};