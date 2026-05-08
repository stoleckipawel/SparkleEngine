#include "PCH.h"

#include "Cooking/TextureCookArtifactKeyBuilder.h"

#include "Constants/TextureCookerConstants.h"

#include "D3D12/Textures/CookedTextureAsset.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"

namespace AssetAuthoring
{
	bool TextureCookArtifactKeyBuilder::TryBuild(
		const TextureCookRequest& request,
		Cook::CookArtifactKey& outKey,
		std::string& outErrorMessage)
	{
		std::uint64_t sourceHash = 0;
		if (!Hash::TryFnv1a64File(request.sourcePath, sourceHash, outErrorMessage))
		{
			return false;
		}

		outKey = Cook::CookArtifactKey{
		    .assetType = "Texture",
		    .assetId = Formatting::FormatHexUInt64(request.assetId),
		    .cookerName = std::string(TextureCookerConstants::ToolName),
		    .outputPath = request.outputPath,
		    .cookedFormatVersion = kCookedTextureAssetVersion,
		    .cookerVersion = TextureCookerConstants::CookerVersion,
		    .sourceHash = sourceHash,
		    .dependencyHash = 0,
		    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash(
		        std::string("ColorSpace=") + GetTextureColorSpaceName(request.colorSpace) +
		        ";MipPolicy=" + GetTextureMipPolicyName(request.mipPolicy) +
		        ";MipFilter=" + GetTextureMipFilterName(request.mipFilter) +
		        ";ColorProcessing=" + GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) +
		        ";CompressionFamily=" + GetTextureCompressionFamilyPreferenceName(request.compressionFamilyPreference) +
		        ";Dimension=" + GetTextureDimensionName(request.dimension))};
		outErrorMessage.clear();
		return true;
	}
}