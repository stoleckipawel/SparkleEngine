#include "PCH.h"

#include "MaterialCooker.h"

#include "CookArtifactCache.h"
#include "TextureCookRequestBuilder.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_set>
#include <utility>

static constexpr std::uint32_t kMaterialCookerVersion = 1;

static Cook::CookArtifactKey BuildMaterialCookArtifactKey(
    const CookedMaterialAssetBuild& materialAsset,
    const std::filesystem::path& outputPath)
{
	std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, materialAsset.header);
	contentHash = Hash::ContinueFnv1a64(contentHash, materialAsset.name.data(), materialAsset.name.size());
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, materialAsset.textureReferences);

	return Cook::CookArtifactKey{
	    .assetType = "Material",
	    .assetId = Formatting::FormatHexUInt64(materialAsset.assetId),
	    .cookerName = "MaterialCooker",
	    .outputPath = outputPath,
	    .cookedFormatVersion = Assets::kCookedMaterialAssetVersion,
	    .cookerVersion = kMaterialCookerVersion,
	    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
	    .dependencyHash = 0,
	    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedMaterialAsset")};
}

bool MaterialCooker::BuildMaterialAssets(
    const SourceImportResult& importResult,
    std::string_view sceneAssetId,
    std::vector<CookedMaterialAssetBuild>& outMaterialAssets,
    std::vector<Assets::CookedSceneMaterialAssetRef>& outMaterialAssetReferences,
    std::string& outErrorMessage)
{
	outMaterialAssets.clear();
	outMaterialAssetReferences.clear();
	outMaterialAssets.reserve(importResult.materials.size());
	outMaterialAssetReferences.reserve(importResult.materials.size());

	auto appendTextureReference = [&](const SourceImportResult::MaterialTextureSource& textureSource,
	                                  CookedMaterialAssetBuild& materialAsset) -> bool
	{
		if (textureSource.sourcePath.empty())
		{
			return true;
		}

		TextureCookRequest request;
		if (!TextureCookRequestBuilder::Build(
		        textureSource.sourcePath,
		        textureSource.textureGroup,
		        request,
		        outErrorMessage,
		        textureSource.channelMask))
		{
			return false;
		}

		materialAsset.textureReferences.push_back({static_cast<Assets::CookedAssetId>(request.assetId), textureSource.textureGroup});
		return true;
	};

	auto appendFallbackTextureReference = [&](const std::optional<std::filesystem::path>& texturePath,
	                                          TextureGroup textureGroup,
	                                          CookedMaterialAssetBuild& materialAsset,
	                                          TextureChannelMask channelMask = TextureChannelMask::Rgba) -> bool
	{
		if (!texturePath)
		{
			return true;
		}

		return appendTextureReference({textureGroup, *texturePath, channelMask}, materialAsset);
	};

	for (std::size_t materialIndex = 0; materialIndex < importResult.materials.size(); ++materialIndex)
	{
		const MaterialDesc& materialDesc = importResult.materials[materialIndex];
		CookedMaterialAssetBuild materialAsset;
		materialAsset.assetId = BuildMaterialAssetId(sceneAssetId, materialIndex);
		materialAsset.name = materialDesc.name;
		materialAsset.header.nameByteCount = static_cast<std::uint32_t>(materialAsset.name.size());
		materialAsset.header.textureReferenceCount = 0;
		materialAsset.header.alphaMode = TranslateAlphaMode(materialDesc.alphaMode);
		materialAsset.header.baseColor = materialDesc.baseColor;
		materialAsset.header.metallic = materialDesc.metallic;
		materialAsset.header.roughness = materialDesc.roughness;
		materialAsset.header.f0 = materialDesc.f0;
		materialAsset.header.subsurfaceColor = materialDesc.subsurfaceColor;
		materialAsset.header.subsurfaceStrength = materialDesc.subsurfaceStrength;
		materialAsset.header.alphaCutoff = materialDesc.alphaCutoff;
		materialAsset.header.emissiveColor = materialDesc.emissiveColor;

		if (materialIndex < importResult.materialTextureSources.size() && !importResult.materialTextureSources[materialIndex].empty())
		{
			for (const SourceImportResult::MaterialTextureSource& textureSource : importResult.materialTextureSources[materialIndex])
			{
				if (!appendTextureReference(textureSource, materialAsset))
				{
					return false;
				}
			}
		}
		else if (
		    !appendFallbackTextureReference(materialDesc.albedoTexture, TextureGroup::Diffuse, materialAsset) ||
		    !appendFallbackTextureReference(materialDesc.normalTexture, TextureGroup::NormalMap, materialAsset) ||
		    !appendFallbackTextureReference(
		        materialDesc.roughnessTexture,
		        TextureGroup::Roughness,
		        materialAsset,
		        TextureChannelMask::Red) ||
		    !appendFallbackTextureReference(
		        materialDesc.metallicTexture,
		        TextureGroup::Metallic,
		        materialAsset,
		        TextureChannelMask::Red) ||
		    !appendFallbackTextureReference(
		        materialDesc.occlusionTexture,
		        TextureGroup::AmbientOcclusion,
		        materialAsset,
		        TextureChannelMask::Red) ||
		    !appendFallbackTextureReference(materialDesc.emissiveTexture, TextureGroup::Emissive, materialAsset) ||
		    !appendFallbackTextureReference(materialDesc.subsurfaceColorTexture, TextureGroup::SubsurfaceColor, materialAsset) ||
		    !appendFallbackTextureReference(
		        materialDesc.subsurfaceStrengthTexture,
		        TextureGroup::SubsurfaceStrength,
		        materialAsset,
		        TextureChannelMask::Red))
		{
			return false;
		}

		materialAsset.header.textureReferenceCount = static_cast<std::uint32_t>(materialAsset.textureReferences.size());

		outMaterialAssetReferences.push_back({materialAsset.assetId});
		outMaterialAssets.push_back(std::move(materialAsset));
	}

	outErrorMessage.clear();
	return true;
}

bool MaterialCooker::CollectTextureCookRequests(
    const SourceImportResult& importResult,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	outRequests.clear();
	std::unordered_set<TextureAssetId> referencedTextureAssetIds;
	referencedTextureAssetIds.reserve(importResult.materials.size() * 8);

	auto appendTextureRequest = [&](const SourceImportResult::MaterialTextureSource& textureSource) -> bool
	{
		if (textureSource.sourcePath.empty())
		{
			return true;
		}

		TextureCookRequest request;
		if (!TextureCookRequestBuilder::Build(
		        textureSource.sourcePath,
		        textureSource.textureGroup,
		        request,
		        outErrorMessage,
		        textureSource.channelMask))
		{
			return false;
		}

		if (referencedTextureAssetIds.insert(request.assetId).second)
		{
			outRequests.push_back(std::move(request));
		}

		return true;
	};

	auto appendFallbackTextureRequest = [&](const std::optional<std::filesystem::path>& texturePath,
	                                        TextureGroup textureGroup,
	                                        TextureChannelMask channelMask = TextureChannelMask::Rgba) -> bool
	{
		if (!texturePath)
		{
			return true;
		}

		return appendTextureRequest({textureGroup, *texturePath, channelMask});
	};

	for (std::size_t materialIndex = 0; materialIndex < importResult.materials.size(); ++materialIndex)
	{
		if (materialIndex < importResult.materialTextureSources.size() && !importResult.materialTextureSources[materialIndex].empty())
		{
			for (const SourceImportResult::MaterialTextureSource& textureSource : importResult.materialTextureSources[materialIndex])
			{
				if (!appendTextureRequest(textureSource))
				{
					return false;
				}
			}

			continue;
		}

		const MaterialDesc& materialDesc = importResult.materials[materialIndex];
		if (!appendFallbackTextureRequest(materialDesc.albedoTexture, TextureGroup::Diffuse) ||
		    !appendFallbackTextureRequest(materialDesc.normalTexture, TextureGroup::NormalMap) ||
		    !appendFallbackTextureRequest(materialDesc.roughnessTexture, TextureGroup::Roughness, TextureChannelMask::Red) ||
		    !appendFallbackTextureRequest(materialDesc.metallicTexture, TextureGroup::Metallic, TextureChannelMask::Red) ||
		    !appendFallbackTextureRequest(materialDesc.occlusionTexture, TextureGroup::AmbientOcclusion, TextureChannelMask::Red) ||
		    !appendFallbackTextureRequest(materialDesc.emissiveTexture, TextureGroup::Emissive) ||
		    !appendFallbackTextureRequest(materialDesc.subsurfaceColorTexture, TextureGroup::SubsurfaceColor) ||
		    !appendFallbackTextureRequest(
		        materialDesc.subsurfaceStrengthTexture,
		        TextureGroup::SubsurfaceStrength,
		        TextureChannelMask::Red))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

bool MaterialCooker::WriteMaterialAssets(
    const std::vector<CookedMaterialAssetBuild>& materialAssets,
    std::string& outErrorMessage)
{
	for (const CookedMaterialAssetBuild& materialAsset : materialAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedMaterialAsset(materialAsset.assetId);
		const Cook::CookArtifactKey artifactKey = BuildMaterialCookArtifactKey(materialAsset, outputPath);
		bool isCurrent = false;
		isCurrent = Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage);
		if (!isCurrent && !outErrorMessage.empty())
		{
			return false;
		}
		if (isCurrent)
		{
			continue;
		}

		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, materialAsset.header, outErrorMessage))
		{
			return false;
		}

		if (!materialAsset.name.empty())
		{
			output.write(materialAsset.name.data(), static_cast<std::streamsize>(materialAsset.name.size()));
			if (!output.good())
			{
				outErrorMessage = "Failed to write cooked material asset name payload";
				return false;
			}
		}

		if (!Files::BinaryStreamWriter::WriteArray(output, materialAsset.textureReferences, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
		{
			return false;
		}

		if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

Assets::CookedAssetId MaterialCooker::BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept
{
	return Hash::Fnv1a64(std::string(sceneAssetId) + "#material#" + std::to_string(materialIndex));
}

Assets::CookedAlphaMode MaterialCooker::TranslateAlphaMode(AlphaMode alphaMode) noexcept
{
	switch (alphaMode)
	{
		case AlphaMode::Opaque:
			return Assets::CookedAlphaMode::Opaque;
		case AlphaMode::Mask:
			return Assets::CookedAlphaMode::Mask;
		case AlphaMode::Blend:
			return Assets::CookedAlphaMode::Blend;
	}

	return Assets::CookedAlphaMode::Opaque;
}
