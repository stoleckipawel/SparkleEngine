#include "PCH.h"

#include "Assets/CookedAssembly/CookedMaterialTranslator.h"

#include "Core/Public/Diagnostics/Error.h"

#include <format>

namespace Assets
{
	MaterialDesc CookedMaterialTranslator::Translate(const LoadedMaterialAsset& materialAsset) const
	{
		MaterialDesc materialDesc;
		materialDesc.name = materialAsset.name;
		materialDesc.baseColor = materialAsset.header.baseColor;
		materialDesc.metallic = materialAsset.header.metallic;
		materialDesc.roughness = materialAsset.header.roughness;
		materialDesc.f0 = materialAsset.header.f0;
		materialDesc.subsurfaceColor = materialAsset.header.subsurfaceColor;
		materialDesc.subsurfaceStrength = materialAsset.header.subsurfaceStrength;
		materialDesc.emissiveColor = materialAsset.header.emissiveColor;
		materialDesc.alphaMode = TranslateAlphaMode(materialAsset.header.alphaMode);
		materialDesc.alphaCutoff = materialAsset.header.alphaCutoff;
		materialDesc.doubleSided = materialAsset.header.doubleSided != 0;
		materialDesc.textureReferences.reserve(materialAsset.textureReferences.size());

		for (const CookedTextureReference& textureReference : materialAsset.textureReferences)
		{
			if (!textureReference.IsValid())
			{
				throw Diagnostics::Error(std::format(
				    "Cooked material '{}' contains an invalid texture path for texture group {}",
				    materialAsset.name,
				    static_cast<std::uint32_t>(textureReference.textureGroup)));
			}

			materialDesc.AddTextureReference(textureReference);
		}

		return materialDesc;
	}

	AlphaMode CookedMaterialTranslator::TranslateAlphaMode(CookedAlphaMode alphaMode)
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

		throw Diagnostics::Error(std::format("Cooked material contains unknown alpha mode {}", static_cast<std::uint32_t>(alphaMode)));
	}

}
