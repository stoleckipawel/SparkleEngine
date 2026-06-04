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
	outOutput.assets.reserve(importResult.scene.materials.size());
	outOutput.assetReferences.reserve(importResult.scene.materials.size());

	auto appendTextureReference = [&](const ImportedTextureSource& textureSource,
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

	for (std::size_t materialIndex = 0; materialIndex < importResult.scene.materials.size(); ++materialIndex)
	{
		const ImportedMaterial& importedMaterial = importResult.scene.materials[materialIndex];
		CookedMaterialAssetBuild materialAsset;
		materialAsset.assetId = BuildMaterialAssetId(sceneAssetId, materialIndex);
		materialAsset.name = importedMaterial.name;
		materialAsset.header.nameByteCount = static_cast<std::uint32_t>(materialAsset.name.size());
		materialAsset.header.textureReferenceCount = 0;
		materialAsset.header.alphaMode = TranslateAlphaMode(importedMaterial.alphaMode);
		materialAsset.header.baseColor = importedMaterial.baseColor;
		materialAsset.header.metallic = importedMaterial.metallic;
		materialAsset.header.roughness = importedMaterial.roughness;
		materialAsset.header.f0 = importedMaterial.f0;
		materialAsset.header.subsurfaceColor = importedMaterial.subsurfaceColor;
		materialAsset.header.subsurfaceStrength = importedMaterial.subsurfaceStrength;
		materialAsset.header.alphaCutoff = importedMaterial.alphaCutoff;
		materialAsset.header.emissiveColor = importedMaterial.emissiveColor;
		materialAsset.header.doubleSided = importedMaterial.doubleSided ? 1u : 0u;

		for (const ImportedTextureSource& textureSource : importedMaterial.textureSources)
		{
			if (!appendTextureReference(textureSource, materialAsset))
			{
				return false;
			}
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

	auto appendTextureRequest = [&](const ImportedTextureSource& textureSource) -> bool
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

	for (std::size_t materialIndex = 0; materialIndex < importResult.scene.materials.size(); ++materialIndex)
	{
		const ImportedMaterial& importedMaterial = importResult.scene.materials[materialIndex];
		for (const ImportedTextureSource& textureSource : importedMaterial.textureSources)
		{
			if (!appendTextureRequest(textureSource))
			{
				return false;
			}
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

Assets::CookedAlphaMode MaterialCooker::TranslateAlphaMode(ImportedAlphaMode alphaMode) noexcept
{
	switch (alphaMode)
	{
		case ImportedAlphaMode::Opaque:
			return Assets::CookedAlphaMode::Opaque;
		case ImportedAlphaMode::Mask:
			return Assets::CookedAlphaMode::Mask;
		case ImportedAlphaMode::Blend:
			return Assets::CookedAlphaMode::Blend;
	}

	return Assets::CookedAlphaMode::Opaque;
}
