#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "MaterialCooker.h"

#include "CookedMaterialAssetBuild.h"
#include "SourceImportResult.h"
#include "TextureCookRequestBuilder.h"
#include "TextureCookRequestList.h"

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

class MaterialCookerOperations final
{
  public:
	static bool BuildCookedTextureReferencePath(
	    const TextureCookRequest& request,
	    std::string& outTextureReferencePath,
	    std::string& outErrorMessage);
	static bool BuildMaterialAsset(
	    const ImportedMaterial& importedMaterial,
	    std::string_view sceneAssetId,
	    std::size_t materialIndex,
	    MaterialCookOutput& outOutput,
	    std::string& outErrorMessage);
	static bool AppendTextureReference(
	    const ImportedTextureSource& textureSource,
	    CookedMaterialAssetBuild& materialAsset,
	    std::string& outErrorMessage);
	static bool AppendTextureRequest(
	    const ImportedTextureSource& textureSource,
	    TextureCookRequestSet& requestSet,
	    std::string& outErrorMessage);
	static bool StageMaterialAsset(
	    const CookedMaterialAssetBuild& materialAsset,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool WriteMaterialAsset(
	    const CookedMaterialAssetBuild& materialAsset,
	    const std::filesystem::path& stagedOutputPath,
	    std::string& outErrorMessage);
	static bool WriteMaterialName(
	    std::ofstream& output,
	    const CookedMaterialAssetBuild& materialAsset,
	    std::string& outErrorMessage);
	static bool BuildTextureReferenceRecords(
	    const CookedMaterialAssetBuild& materialAsset,
	    std::vector<Assets::CookedTextureReferenceRecord>& outRecords,
	    std::string& outErrorMessage);
	static bool WriteTextureReferencePaths(
	    std::ofstream& output,
	    const CookedMaterialAssetBuild& materialAsset,
	    std::string& outErrorMessage);
	static Assets::CookedAssetId BuildMaterialAssetId(
	    std::string_view sceneAssetId,
	    std::size_t materialIndex) noexcept;
	static Assets::CookedAlphaMode TranslateAlphaMode(
	    ImportedAlphaMode alphaMode) noexcept;
};

bool MaterialCookerOperations::BuildCookedTextureReferencePath(
    const TextureCookRequest& request,
    std::string& outTextureReferencePath,
    std::string& outErrorMessage)
{
	const std::optional<std::filesystem::path> relativePath =
	    Paths::TryMakeRelativeUnderRoot(
	        request.outputPath,
	        Filesystem::GetCookedAssetRootPath());
	if (!relativePath)
	{
		outErrorMessage =
		    "Cooked texture output path is outside the cooked asset root: '" +
		    request.outputPath.string() + "'";
		return false;
	}

	outTextureReferencePath = relativePath->generic_string();
	return true;
}

bool MaterialCookerOperations::BuildMaterialAsset(
    const ImportedMaterial& importedMaterial,
    std::string_view sceneAssetId,
    std::size_t materialIndex,
    MaterialCookOutput& outOutput,
    std::string& outErrorMessage)
{
	CookedMaterialAssetBuild materialAsset;
	materialAsset.assetId =
	    BuildMaterialAssetId(sceneAssetId, materialIndex);
	materialAsset.name = importedMaterial.name;
	materialAsset.header.nameByteCount =
	    static_cast<std::uint32_t>(materialAsset.name.size());
	materialAsset.header.alphaMode =
	    TranslateAlphaMode(importedMaterial.alphaMode);
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
		if (!AppendTextureReference(
		        textureSource,
		        materialAsset,
		        outErrorMessage))
		{
			return false;
		}
	}

	materialAsset.header.textureReferenceCount =
	    static_cast<std::uint32_t>(materialAsset.textureReferences.size());
	outOutput.assetReferences.push_back({materialAsset.assetId});
	outOutput.assets.push_back(std::move(materialAsset));
	return true;
}

bool MaterialCookerOperations::AppendTextureReference(
    const ImportedTextureSource& textureSource,
    CookedMaterialAssetBuild& materialAsset,
    std::string& outErrorMessage)
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
	if (!BuildCookedTextureReferencePath(
	        request,
	        textureReferencePath,
	        outErrorMessage))
	{
		return false;
	}

	materialAsset.textureReferences.push_back(
	    {std::move(textureReferencePath), textureSource.textureGroup});
	return true;
}

bool MaterialCookerOperations::AppendTextureRequest(
    const ImportedTextureSource& textureSource,
    TextureCookRequestSet& requestSet,
    std::string& outErrorMessage)
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

	return requestSet.Add(request, outErrorMessage);
}

bool MaterialCookerOperations::StageMaterialAsset(
    const CookedMaterialAssetBuild& materialAsset,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	const std::filesystem::path outputPath =
	    Paths::CookedMaterialAsset(materialAsset.assetId);
	const std::filesystem::path stagedOutputPath =
	    Files::BuildTemporaryPath(outputPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	outPublication.push_back({stagedOutputPath, outputPath});

	return WriteMaterialAsset(
	    materialAsset,
	    stagedOutputPath,
	    outErrorMessage);
}

bool MaterialCookerOperations::WriteMaterialAsset(
    const CookedMaterialAssetBuild& materialAsset,
    const std::filesystem::path& stagedOutputPath,
    std::string& outErrorMessage)
{
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteValue(
	        output,
	        materialAsset.header,
	        outErrorMessage))
	{
		return false;
	}

	if (!WriteMaterialName(output, materialAsset, outErrorMessage))
	{
		return false;
	}

	std::vector<Assets::CookedTextureReferenceRecord> records;
	if (!BuildTextureReferenceRecords(materialAsset, records, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, records, outErrorMessage) ||
	    !WriteTextureReferencePaths(output, materialAsset, outErrorMessage))
	{
		return false;
	}

	return Files::TryCloseOutput(output, stagedOutputPath, outErrorMessage);
}

bool MaterialCookerOperations::WriteMaterialName(
    std::ofstream& output,
    const CookedMaterialAssetBuild& materialAsset,
    std::string& outErrorMessage)
{
	if (materialAsset.name.empty())
	{
		return true;
	}

	output.write(
	    materialAsset.name.data(),
	    static_cast<std::streamsize>(materialAsset.name.size()));
	if (!output.good())
	{
		outErrorMessage = "Failed to write cooked material asset name payload";
		return false;
	}

	return true;
}

bool MaterialCookerOperations::BuildTextureReferenceRecords(
    const CookedMaterialAssetBuild& materialAsset,
    std::vector<Assets::CookedTextureReferenceRecord>& outRecords,
    std::string& outErrorMessage)
{
	outRecords.reserve(materialAsset.textureReferences.size());
	for (const Assets::CookedTextureReference& reference : materialAsset.textureReferences)
	{
		if (reference.texturePath.size() > (std::numeric_limits<std::uint32_t>::max)())
		{
			outErrorMessage = "Cooked material texture reference path is too large to serialize";
			return false;
		}

		outRecords.push_back(
		    {.texturePathByteCount = static_cast<std::uint32_t>(reference.texturePath.size()),
		     .textureGroup = reference.textureGroup});
	}

	return true;
}

bool MaterialCookerOperations::WriteTextureReferencePaths(
    std::ofstream& output,
    const CookedMaterialAssetBuild& materialAsset,
    std::string& outErrorMessage)
{
	for (const Assets::CookedTextureReference& reference : materialAsset.textureReferences)
	{
		if (!Files::BinaryStreamWriter::WriteBytes(
		        output,
		        reference.texturePath.data(),
		        reference.texturePath.size(),
		        outErrorMessage))
		{
			return false;
		}
	}

	return true;
}

Assets::CookedAssetId MaterialCookerOperations::BuildMaterialAssetId(
    std::string_view sceneAssetId,
    std::size_t materialIndex) noexcept
{
	return Hash::Fnv1a64(
	    std::string(sceneAssetId) +
	    "#material#" +
	    std::to_string(materialIndex));
}

Assets::CookedAlphaMode MaterialCookerOperations::TranslateAlphaMode(
    ImportedAlphaMode alphaMode) noexcept
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

	for (std::size_t materialIndex = 0; materialIndex < importResult.scene.materials.size(); ++materialIndex)
	{
		if (!MaterialCookerOperations::BuildMaterialAsset(
		        importResult.scene.materials[materialIndex],
		        sceneAssetId,
		        materialIndex,
		        outOutput,
		        outErrorMessage))
		{
			return false;
		}
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

	for (const ImportedMaterial& importedMaterial : importResult.scene.materials)
	{
		for (const ImportedTextureSource& textureSource : importedMaterial.textureSources)
		{
			if (!MaterialCookerOperations::AppendTextureRequest(
			        textureSource,
			        requestSet,
			        outErrorMessage))
			{
				return false;
			}
		}
	}

	requestSet.MoveRequestsTo(outRequests);
	outErrorMessage.clear();
	return true;
}

bool MaterialCooker::StageMaterialAssets(
    const std::vector<CookedMaterialAssetBuild>& materialAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedMaterialAssetBuild& materialAsset : materialAssets)
	{
		if (!MaterialCookerOperations::StageMaterialAsset(
		        materialAsset,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

