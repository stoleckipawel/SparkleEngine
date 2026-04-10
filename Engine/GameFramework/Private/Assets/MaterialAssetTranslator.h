#pragma once

#include "Assets/Loaders/LoadedCookedAssets.h"
#include "Scene/Materials/MaterialDesc.h"

#include <filesystem>
#include <string>

namespace Engine::Assets
{
	class MaterialAssetTranslator final
	{
	  public:
		bool Translate(const LoadedMaterialAsset& materialAsset, MaterialDesc& outMaterialDesc, std::string& outErrorMessage) const;

	  private:
		static AlphaMode TranslateAlphaMode(CookedAlphaMode alphaMode) noexcept;
		static MaterialTextureType TranslateTextureType(CookedTextureSemantic semantic) noexcept;
		static std::filesystem::path BuildCookedTextureAssetPath(CookedAssetId textureAssetId);
	};
}