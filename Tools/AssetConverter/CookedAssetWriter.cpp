#include "CookedAssetWriter.h"

#include "Log.h"

#include <fstream>
#include <format>

CookedAssetWriteResult CookedAssetWriter::Write(
	const SceneImportResult& result,
	const std::filesystem::path& outputDirectory)
{
	CookedAssetWriteResult writeResult;
	if (!result.IsValid())
	{
		writeResult.errorMessage = "CookedAssetWriter: Scene import result is invalid";
		return writeResult;
	}

	std::error_code errorCode;
	std::filesystem::create_directories(outputDirectory, errorCode);
	if (errorCode)
	{
		writeResult.errorMessage = std::format(
		    "CookedAssetWriter: Failed to create output directory '{}' ({})",
		    outputDirectory.string(),
		    errorCode.message());
		return writeResult;
	}

	const std::string assetStem = SanitizeIdentifier(result.stats.sourcePath.stem().string());
	const std::filesystem::path meshPath = outputDirectory / std::format("{}.smesh", assetStem.empty() ? "scene" : assetStem);
	const std::filesystem::path materialPath = outputDirectory / std::format("{}.smat", assetStem.empty() ? "scene" : assetStem);
	const std::filesystem::path textureManifestPath = outputDirectory / std::format("{}.stex", assetStem.empty() ? "scene" : assetStem);
	const std::filesystem::path sceneAssetPath = outputDirectory / std::format("{}.sasset", assetStem.empty() ? "scene" : assetStem);

	CookedAssetBuildOptions options;
	options.assetStem = assetStem;
	options.meshAssetRelativePath = meshPath.filename();
	options.materialAssetRelativePath = materialPath.filename();
	options.textureManifestRelativePath = textureManifestPath.filename();
	options.cookedTextureDirectory = "textures";

	const CookedAssetPackageDefinition packageDefinition = CookedAssetFormatBuilder::BuildFromSceneImportResult(result, options);
	if (!packageDefinition.IsValid())
	{
		writeResult.errorMessage = "CookedAssetWriter: Failed to build cooked asset package definition";
		return writeResult;
	}

	std::filesystem::create_directories(outputDirectory / options.cookedTextureDirectory, errorCode);
	if (errorCode)
	{
		writeResult.errorMessage = std::format(
		    "CookedAssetWriter: Failed to create cooked texture directory '{}' ({})",
		    (outputDirectory / options.cookedTextureDirectory).string(),
		    errorCode.message());
		return writeResult;
	}

	if (!WriteMeshAsset(packageDefinition.meshAsset, meshPath, writeResult.errorMessage) ||
	    !WriteMaterialAsset(packageDefinition.materialAsset, materialPath, writeResult.errorMessage) ||
	    !WriteTextureManifest(packageDefinition.textureManifest, textureManifestPath, writeResult.errorMessage) ||
	    !WriteSceneAsset(packageDefinition.sceneAsset, sceneAssetPath, writeResult.errorMessage))
	{
		return writeResult;
	}

	writeResult.bSuccess = true;
	writeResult.sceneAssetPath = sceneAssetPath;
	return writeResult;
}

bool CookedAssetWriter::WriteSceneAsset(
	const CookedSceneAssetDefinition& sceneAsset,
	const std::filesystem::path& outputPath,
	std::string& errorMessage)
{
	std::ofstream outputStream;
	if (!OpenOutputFile(outputPath, outputStream, errorMessage))
	{
		return false;
	}

	if (!WriteSceneAssetPayload(outputStream, sceneAsset))
	{
		errorMessage = std::format("CookedAssetWriter: Failed to write scene asset '{}'", outputPath.string());
		return false;
	}

	return true;
}

bool CookedAssetWriter::WriteMeshAsset(
	const CookedMeshAssetDefinition& meshAsset,
	const std::filesystem::path& outputPath,
	std::string& errorMessage)
{
	std::ofstream outputStream;
	if (!OpenOutputFile(outputPath, outputStream, errorMessage))
	{
		return false;
	}

	if (!WriteMeshAssetPayload(outputStream, meshAsset))
	{
		errorMessage = std::format("CookedAssetWriter: Failed to write mesh asset '{}'", outputPath.string());
		return false;
	}

	return true;
}

bool CookedAssetWriter::WriteMaterialAsset(
	const CookedMaterialAssetDefinition& materialAsset,
	const std::filesystem::path& outputPath,
	std::string& errorMessage)
{
	std::ofstream outputStream;
	if (!OpenOutputFile(outputPath, outputStream, errorMessage))
	{
		return false;
	}

	if (!WriteMaterialAssetPayload(outputStream, materialAsset))
	{
		errorMessage = std::format("CookedAssetWriter: Failed to write material asset '{}'", outputPath.string());
		return false;
	}

	return true;
}

bool CookedAssetWriter::WriteTextureManifest(
	const CookedTextureManifestDefinition& textureManifest,
	const std::filesystem::path& outputPath,
	std::string& errorMessage)
{
	std::ofstream outputStream;
	if (!OpenOutputFile(outputPath, outputStream, errorMessage))
	{
		return false;
	}

	if (!WriteTextureManifestPayload(outputStream, textureManifest))
	{
		errorMessage = std::format("CookedAssetWriter: Failed to write texture manifest '{}'", outputPath.string());
		return false;
	}

	return true;
}

bool CookedAssetWriter::OpenOutputFile(
	const std::filesystem::path& outputPath,
	std::ofstream& outputStream,
	std::string& errorMessage)
{
	outputStream.open(outputPath, std::ios::binary | std::ios::trunc);
	if (outputStream.is_open())
	{
		return true;
	}

	errorMessage = std::format("CookedAssetWriter: Failed to open '{}' for writing", outputPath.string());
	return false;
}

bool CookedAssetWriter::WriteSceneAssetPayload(std::ofstream& outputStream, const CookedSceneAssetDefinition& sceneAsset)
{
	outputStream.write(reinterpret_cast<const char*>(&sceneAsset.header), sizeof(sceneAsset.header));
	outputStream.write(
	    reinterpret_cast<const char*>(sceneAsset.meshEntries.data()),
	    static_cast<std::streamsize>(sceneAsset.meshEntries.size() * sizeof(CookedSceneMeshEntry)));
	outputStream.write(
	    reinterpret_cast<const char*>(sceneAsset.references.data()),
	    static_cast<std::streamsize>(sceneAsset.references.size() * sizeof(CookedAssetReferenceEntry)));
	outputStream.write(sceneAsset.stringTable.data(), static_cast<std::streamsize>(sceneAsset.stringTable.size()));
	return outputStream.good();
}

bool CookedAssetWriter::WriteMeshAssetPayload(std::ofstream& outputStream, const CookedMeshAssetDefinition& meshAsset)
{
	outputStream.write(reinterpret_cast<const char*>(&meshAsset.header), sizeof(meshAsset.header));
	outputStream.write(
	    reinterpret_cast<const char*>(meshAsset.meshTable.data()),
	    static_cast<std::streamsize>(meshAsset.meshTable.size() * sizeof(CookedMeshEntry)));
	outputStream.write(
	    reinterpret_cast<const char*>(meshAsset.vertexBlob.data()),
	    static_cast<std::streamsize>(meshAsset.vertexBlob.size() * sizeof(VertexData)));
	outputStream.write(
	    reinterpret_cast<const char*>(meshAsset.indexBlob.data()),
	    static_cast<std::streamsize>(meshAsset.indexBlob.size() * sizeof(std::uint32_t)));
	return outputStream.good();
}

bool CookedAssetWriter::WriteMaterialAssetPayload(std::ofstream& outputStream, const CookedMaterialAssetDefinition& materialAsset)
{
	outputStream.write(reinterpret_cast<const char*>(&materialAsset.header), sizeof(materialAsset.header));
	outputStream.write(
	    reinterpret_cast<const char*>(materialAsset.materials.data()),
	    static_cast<std::streamsize>(materialAsset.materials.size() * sizeof(CookedMaterialEntry)));
	outputStream.write(materialAsset.stringTable.data(), static_cast<std::streamsize>(materialAsset.stringTable.size()));
	return outputStream.good();
}

bool CookedAssetWriter::WriteTextureManifestPayload(std::ofstream& outputStream, const CookedTextureManifestDefinition& textureManifest)
{
	outputStream.write(reinterpret_cast<const char*>(&textureManifest.header), sizeof(textureManifest.header));
	outputStream.write(
	    reinterpret_cast<const char*>(textureManifest.textures.data()),
	    static_cast<std::streamsize>(textureManifest.textures.size() * sizeof(CookedTextureEntry)));
	outputStream.write(textureManifest.stringTable.data(), static_cast<std::streamsize>(textureManifest.stringTable.size()));
	return outputStream.good();
}

std::filesystem::path CookedAssetWriter::GetReferencePath(
	const std::vector<char>& stringTable,
	const CookedStringRef& stringRef,
	const std::filesystem::path& fallbackPath)
{
	const std::string_view referenceValue = GetStringView(stringTable, stringRef);
	if (referenceValue.empty())
	{
		return fallbackPath;
	}

	return std::filesystem::path(referenceValue);
}

std::string_view CookedAssetWriter::GetStringView(
	const std::vector<char>& stringTable,
	const CookedStringRef& stringRef) noexcept
{
	if (stringRef.length == 0 || stringRef.offset + stringRef.length > stringTable.size())
	{
		return {};
	}

	return std::string_view(stringTable.data() + stringRef.offset, stringRef.length);
}

std::string CookedAssetWriter::SanitizeIdentifier(std::string_view identifier)
{
	std::string sanitized;
	sanitized.reserve(identifier.size());

	for (const char character : identifier)
	{
		if ((character >= 'a' && character <= 'z') ||
		    (character >= 'A' && character <= 'Z') ||
		    (character >= '0' && character <= '9'))
		{
			sanitized.push_back(character);
			continue;
		}

		if (!sanitized.empty() && sanitized.back() != '_')
		{
			sanitized.push_back('_');
		}
	}

	if (!sanitized.empty() && sanitized.back() == '_')
	{
		sanitized.pop_back();
	}

	return sanitized.empty() ? std::string("scene") : sanitized;
}
