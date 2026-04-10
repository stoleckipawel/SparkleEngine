#pragma once

#include <filesystem>
#include <string>

namespace Engine::Assets
{
	struct LoadedMaterialAsset;

	class MaterialAssetLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedMaterialAsset& outMaterialAsset, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(const LoadedMaterialAsset& materialAsset) noexcept;
	};
}