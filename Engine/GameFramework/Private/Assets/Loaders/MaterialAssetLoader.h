#pragma once

#include <filesystem>
#include <string>
#include <span>
#include <cstdint>

namespace Assets
{
	struct LoadedMaterialAsset;

	class MaterialAssetLoader final
	{
	  public:
		bool Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes,
		    LoadedMaterialAsset& outMaterialAsset,
		    std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(const LoadedMaterialAsset& materialAsset) noexcept;
	};
}
