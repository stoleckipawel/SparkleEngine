#include "PCH.h"

#include "GameFramework/Public/Assets/SceneImporter.h"

#include "Assets/FbxImporter.h"
#include "Assets/GltfLoader.h"
#include "Assets/SceneImportPostProcessor.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <format>
#include <string>

SceneImportResult SceneImporter::Load(const std::filesystem::path& filePath)
{
	const auto startTime = std::chrono::steady_clock::now();
	std::string importerName;
	SceneImportResult result;

	std::string extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});

	if (extension == ".gltf" || extension == ".glb")
	{
		importerName = "GltfLoader";
		result = GltfLoader::Load(filePath);
	}
	else if (extension == ".fbx")
	{
		importerName = "FbxImporter";
		result = FbxImporter::Load(filePath);
	}
	else
	{
		importerName = "SceneImporter";
		result.errorMessage = std::format(
		    "SceneImporter: Unsupported asset extension '{}' for '{}'",
		    extension.empty() ? std::string("<none>") : extension,
		    filePath.string());
	}

	result.stats.importerName = std::move(importerName);
	result.stats.sourcePath = filePath;

	if (result.bSuccess)
	{
		SceneImportPostProcessor::Finalize(result);
	}

	const auto endTime = std::chrono::steady_clock::now();
	result.stats.importDurationMs =
	    std::chrono::duration<double, std::milli>(endTime - startTime).count();

	return result;
}