#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "MaterialCooker.h"

#include "CookedMaterialAssetBuild.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedMaterialAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"
#include "SourceImportOutput.h"
#include "TextureCookRequestBuilder.h"
#include "TextureCookRequestList.h"
#include "Types/ImportedMaterial.h"
#include "Types/ImportedTextureSource.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class MaterialCookPipeline final
{
public:
	static std::string BuildCookedTextureReferencePath(const TextureCookRequest& request);
	static CookedMaterialAssetBuild BuildMaterialAsset(
	    const ImportedMaterial& importedMaterial,
	    std::string_view sceneAssetId,
	    std::size_t materialIndex);
	static void AppendTextureReference(const ImportedTextureSource& textureSource, CookedMaterialAssetBuild& materialAsset);
	static void AppendTextureRequest(const ImportedTextureSource& textureSource, TextureCookRequestSet& requestSet);
	static void StageMaterialAsset(const CookedMaterialAssetBuild& materialAsset, std::vector<Files::FilePublication>& outPublication);
	static void WriteMaterialAsset(const CookedMaterialAssetBuild& materialAsset, const std::filesystem::path& stagedOutputPath);
	static void WriteMaterialName(std::ofstream& output, const CookedMaterialAssetBuild& materialAsset);
	static std::vector<Assets::CookedTextureReferenceRecord> BuildTextureReferenceRecords(const CookedMaterialAssetBuild& materialAsset);
	static void WriteTextureReferencePaths(std::ofstream& output, const CookedMaterialAssetBuild& materialAsset);
	static Assets::CookedAssetId BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept;
	static Assets::CookedAlphaMode TranslateAlphaMode(ImportedAlphaMode alphaMode);
};

std::string MaterialCookPipeline::BuildCookedTextureReferencePath(const TextureCookRequest& request)
{
	const std::optional<std::filesystem::path> relativePath =
	    Paths::TryMakeRelativeUnderRoot(request.outputPath, Filesystem::GetCookedAssetRootPath());
	if (!relativePath)
	{
		throw Diagnostics::Error("Cooked texture output path is outside the cooked asset root: '" + request.outputPath.string() + "'.");
	}

	return relativePath->generic_string();
}

CookedMaterialAssetBuild MaterialCookPipeline::BuildMaterialAsset(
    const ImportedMaterial& importedMaterial,
    std::string_view sceneAssetId,
    std::size_t materialIndex)
{
	CookedMaterialAssetBuild materialAsset;
	materialAsset.assetId = BuildMaterialAssetId(sceneAssetId, materialIndex);
	materialAsset.name = importedMaterial.name;
	materialAsset.header.nameByteCount = static_cast<std::uint32_t>(materialAsset.name.size());
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
		AppendTextureReference(textureSource, materialAsset);
	}

	materialAsset.header.textureReferenceCount = static_cast<std::uint32_t>(materialAsset.textureReferences.size());
	return materialAsset;
}

void MaterialCookPipeline::AppendTextureReference(const ImportedTextureSource& textureSource, CookedMaterialAssetBuild& materialAsset)
{
	if (textureSource.sourcePath.empty())
	{
		return;
	}

	const TextureCookRequest request =
	    TextureCookRequestBuilder::Build(textureSource.sourcePath, textureSource.textureGroup, textureSource.channelMask);

	materialAsset.textureReferences.push_back({BuildCookedTextureReferencePath(request), textureSource.textureGroup});
}

void MaterialCookPipeline::AppendTextureRequest(const ImportedTextureSource& textureSource, TextureCookRequestSet& requestSet)
{
	if (textureSource.sourcePath.empty())
	{
		return;
	}

	const TextureCookRequest request =
	    TextureCookRequestBuilder::Build(textureSource.sourcePath, textureSource.textureGroup, textureSource.channelMask);
	requestSet.Add(request);
}

void MaterialCookPipeline::StageMaterialAsset(
    const CookedMaterialAssetBuild& materialAsset,
    std::vector<Files::FilePublication>& outPublication)
{
	const std::filesystem::path outputPath = Paths::CookedMaterialAsset(materialAsset.assetId);
	const std::filesystem::path stagedOutputPath = Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	WriteMaterialAsset(materialAsset, stagedOutputPath);
}

void MaterialCookPipeline::WriteMaterialAsset(const CookedMaterialAssetBuild& materialAsset, const std::filesystem::path& stagedOutputPath)
{
	std::string errorMessage;
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}
	if (!Files::BinaryStreamWriter::WriteValue(output, materialAsset.header, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}

	WriteMaterialName(output, materialAsset);
	const std::vector<Assets::CookedTextureReferenceRecord> records = BuildTextureReferenceRecords(materialAsset);
	if (!Files::BinaryStreamWriter::WriteArray(output, records, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}
	WriteTextureReferencePaths(output, materialAsset);
	if (!Files::TryCloseOutput(output, stagedOutputPath, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}
}

void MaterialCookPipeline::WriteMaterialName(std::ofstream& output, const CookedMaterialAssetBuild& materialAsset)
{
	if (materialAsset.name.empty())
	{
		return;
	}

	output.write(materialAsset.name.data(), static_cast<std::streamsize>(materialAsset.name.size()));
	if (!output.good())
	{
		throw Diagnostics::Error("Failed to write cooked material asset name payload.");
	}
}

std::vector<Assets::CookedTextureReferenceRecord> MaterialCookPipeline::BuildTextureReferenceRecords(
    const CookedMaterialAssetBuild& materialAsset)
{
	std::vector<Assets::CookedTextureReferenceRecord> records;
	records.reserve(materialAsset.textureReferences.size());
	for (const Assets::CookedTextureReference& reference : materialAsset.textureReferences)
	{
		if (reference.texturePath.size() > (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error("Cooked material texture reference path is too large to serialize.");
		}

		records.push_back(
		    {.texturePathByteCount = static_cast<std::uint32_t>(reference.texturePath.size()), .textureGroup = reference.textureGroup});
	}

	return records;
}

void MaterialCookPipeline::WriteTextureReferencePaths(std::ofstream& output, const CookedMaterialAssetBuild& materialAsset)
{
	std::string errorMessage;
	for (const Assets::CookedTextureReference& reference : materialAsset.textureReferences)
	{
		if (!Files::BinaryStreamWriter::WriteBytes(output, reference.texturePath.data(), reference.texturePath.size(), errorMessage))
		{
			throw Diagnostics::Error(errorMessage);
		}
	}
}

Assets::CookedAssetId MaterialCookPipeline::BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept
{
	return Hash::Fnv1a64(std::string(sceneAssetId) + "#material#" + std::to_string(materialIndex));
}

Assets::CookedAlphaMode MaterialCookPipeline::TranslateAlphaMode(ImportedAlphaMode alphaMode)
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

	throw Diagnostics::Error("Imported material uses an unsupported alpha mode.");
}

MaterialCookOutput MaterialCooker::BuildMaterialAssets(const SourceImportOutput& importOutput, std::string_view sceneAssetId)
{
	MaterialCookOutput output;
	output.assets.reserve(importOutput.scene.materials.size());
	output.assetReferences.reserve(importOutput.scene.materials.size());

	for (std::size_t materialIndex = 0; materialIndex < importOutput.scene.materials.size(); ++materialIndex)
	{
		CookedMaterialAssetBuild materialAsset =
		    MaterialCookPipeline::BuildMaterialAsset(importOutput.scene.materials[materialIndex], sceneAssetId, materialIndex);
		output.assetReferences.push_back({materialAsset.assetId});
		output.assets.push_back(std::move(materialAsset));
	}

	return output;
}

std::vector<TextureCookRequest> MaterialCooker::CollectTextureCookRequests(const SourceImportOutput& importOutput)
{
	TextureCookRequestSet requestSet;

	for (const ImportedMaterial& importedMaterial : importOutput.scene.materials)
	{
		for (const ImportedTextureSource& textureSource : importedMaterial.textureSources)
		{
			MaterialCookPipeline::AppendTextureRequest(textureSource, requestSet);
		}
	}

	return requestSet.ReleaseRequests();
}

void MaterialCooker::StageMaterialAssets(
    const std::vector<CookedMaterialAssetBuild>& materialAssets,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedMaterialAssetBuild& materialAsset : materialAssets)
	{
		MaterialCookPipeline::StageMaterialAsset(materialAsset, outPublication);
	}
}
