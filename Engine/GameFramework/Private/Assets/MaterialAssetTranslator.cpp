#include "PCH.h"

#include "Assets/MaterialAssetTranslator.h"

#include <format>

namespace Assets
{
	bool MaterialAssetTranslator::Translate(
	    const LoadedMaterialAsset& materialAsset,
	    MaterialDesc& outMaterialDesc,
	    std::string& outErrorMessage) const
	{
		outMaterialDesc = MaterialDesc{};
		outMaterialDesc.name = materialAsset.name;
		outMaterialDesc.baseColor = materialAsset.header.baseColor;
		outMaterialDesc.metallic = materialAsset.header.metallic;
		outMaterialDesc.roughness = materialAsset.header.roughness;
		outMaterialDesc.f0 = materialAsset.header.f0;
		outMaterialDesc.subsurfaceColor = materialAsset.header.subsurfaceColor;
		outMaterialDesc.subsurfaceStrength = materialAsset.header.subsurfaceStrength;
		outMaterialDesc.emissiveColor = materialAsset.header.emissiveColor;
		outMaterialDesc.alphaMode = TranslateAlphaMode(materialAsset.header.alphaMode);
		outMaterialDesc.alphaCutoff = materialAsset.header.alphaCutoff;

		for (const CookedTextureReference& textureReference : materialAsset.textureReferences)
		{
			if (!textureReference.IsValid())
			{
				outErrorMessage = std::format(
				    "Cooked material '{}' contains an invalid texture path for texture group {}",
				    materialAsset.name,
				    static_cast<std::uint32_t>(textureReference.textureGroup));
				return false;
			}

			outMaterialDesc.SetTexturePath(textureReference.textureGroup, textureReference.texturePath);
		}

		outErrorMessage.clear();
		return true;
	}

	AlphaMode MaterialAssetTranslator::TranslateAlphaMode(CookedAlphaMode alphaMode) noexcept
	{
		switch (alphaMode)
		{
			case CookedAlphaMode::Opaque:
				return AlphaMode::Opaque;
			case CookedAlphaMode::Mask:
				return AlphaMode::Mask;
			case CookedAlphaMode::Blend:
				return AlphaMode::Blend;
		}

		return AlphaMode::Opaque;
	}

}