#include "PCH.h"

#include "MaterialCooker.h"

#include "TextureCookRequestBuilder.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <utility>

namespace
{
	bool BuildCookedTextureReferencePath(
	    const TextureCookRequest& request,
	    std::string& outTextureReferencePath,
	    std::string& outErrorMessage)
	{
		const std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(request.outputPath, Paths::CookedAssetRoot());
		if (!relativePath)
		{
			outErrorMessage = "Cooked texture output path is outside the cooked asset root: '" + request.outputPath.string() + "'";
			return false;
		}

		outTextureReferencePath = relativePath->generic_string();
		return true;
	}
}

bool MaterialCooker::BuildMaterialAssets(
    const SourceImportResult& importResult,
    std::string_view sceneAssetId,
    MaterialCookOutput& outOutput,
    std::string& outErrorMessage)
{
	outOutput.assets.clear();
	outOutput.assetReferences.clear();
	outOutput.assets.reserve(importResult.materials.size());
	outOutput.assetReferences.reserve(importResult.materials.size());

	auto appendTextureReference = [&](const SourceImportResult::TextureSource& textureSource,
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

		std::string textureReferencePath;
		if (!BuildCookedTextureReferencePath(request, textureReferencePath, outErrorMessage))
		{
			return false;
		}

		materialAsset.textureReferences.push_back({std::move(textureReferencePath), textureSource.textureGroup});
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
		const SourceImportResult::MaterialEntry& materialEntry = importResult.materials[materialIndex];
		const MaterialDesc& materialDesc = materialEntry.description;
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

		if (!materialEntry.textures.empty())
		{
			for (const SourceImportResult::TextureSource& textureSource : materialEntry.textures)
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

		outOutput.assetReferences.push_back({materialAsset.assetId});
		outOutput.assets.push_back(std::move(materialAsset));
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
	TextureCookRequestSet requestSet;

	auto appendTextureRequest = [&](const SourceImportResult::TextureSource& textureSource) -> bool
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

		if (!requestSet.Add(request, outErrorMessage))
		{
			return false;
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
		const SourceImportResult::MaterialEntry& materialEntry = importResult.materials[materialIndex];
		if (!materialEntry.textures.empty())
		{
			for (const SourceImportResult::TextureSource& textureSource : materialEntry.textures)
			{
				if (!appendTextureRequest(textureSource))
				{
					return false;
				}
			}

			continue;
		}

		const MaterialDesc& materialDesc = materialEntry.description;
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

	requestSet.MoveRequestsTo(outRequests);
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

		std::vector<Assets::CookedTextureReferenceRecord> textureReferenceRecords;
		textureReferenceRecords.reserve(materialAsset.textureReferences.size());
		for (const Assets::CookedTextureReference& textureReference : materialAsset.textureReferences)
		{
			if (textureReference.texturePath.size() > (std::numeric_limits<std::uint32_t>::max)())
			{
				outErrorMessage = "Cooked material texture reference path is too large to serialize";
				return false;
			}

			textureReferenceRecords.push_back(
			    {.texturePathByteCount = static_cast<std::uint32_t>(textureReference.texturePath.size()),
			     .textureGroup = textureReference.textureGroup});
		}

		if (!Files::BinaryStreamWriter::WriteArray(output, textureReferenceRecords, outErrorMessage))
		{
			return false;
		}

		for (const Assets::CookedTextureReference& textureReference : materialAsset.textureReferences)
		{
			if (!Files::BinaryStreamWriter::WriteBytes(
			        output,
			        textureReference.texturePath.data(),
			        textureReference.texturePath.size(),
			        outErrorMessage))
			{
				return false;
			}
		}

		if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
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
