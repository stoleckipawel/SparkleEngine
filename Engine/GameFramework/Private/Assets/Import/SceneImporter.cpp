#include "PCH.h"

#include "GameFramework/Public/Assets/Import/SceneImporter.h"

#include "Assets/Importers/FbxImporter.h"
#include "Assets/Importers/GltfLoader.h"
#include "Assets/Import/SceneImportPostProcessor.h"
#include "Core/Public/Strings/StringUtils.h"

#include <chrono>
#include <format>
#include <string>

SceneImportResult SceneImporter::Load(const std::filesystem::path& filePath)
{
	const auto startTime = std::chrono::steady_clock::now();
	std::string importerName;
	SceneImportResult result;

	std::string extension = Engine::Strings::ToLowerCopy(filePath.extension().string());

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
		importerName = "Unsupported";

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
	result.stats.importDurationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

	return result;
}