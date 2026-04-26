#include "PCH.h"

#include "Assets/MaterialAssetTranslator.h"

#include "Core/Public/Paths/DirectoryPaths.h"

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
		outMaterialDesc.emissiveColor = materialAsset.header.emissiveColor;
		outMaterialDesc.alphaMode = TranslateAlphaMode(materialAsset.header.alphaMode);
		outMaterialDesc.alphaCutoff = materialAsset.header.alphaCutoff;

		for (const CookedTextureReference& textureReference : materialAsset.textureReferences)
		{
			if (textureReference.textureAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format(
				    "Cooked material '{}' contains an invalid texture asset id for semantic {}",
				    materialAsset.name,
				    static_cast<std::uint32_t>(textureReference.semantic));
				return false;
			}

			outMaterialDesc.SetTexturePath(
			    TranslateTextureType(textureReference.semantic),
			    Paths::CookedTextureAsset(textureReference.textureAssetId));
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

	MaterialTextureType MaterialAssetTranslator::TranslateTextureType(CookedTextureSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case CookedTextureSemantic::Albedo:
				return MaterialTextureType::Albedo;
			case CookedTextureSemantic::Normal:
				return MaterialTextureType::Normal;
			case CookedTextureSemantic::MetallicRoughness:
				return MaterialTextureType::MetallicRoughness;
			case CookedTextureSemantic::Occlusion:
				return MaterialTextureType::Occlusion;
			case CookedTextureSemantic::Emissive:
				return MaterialTextureType::Emissive;
		}

		return MaterialTextureType::Albedo;
	}

}