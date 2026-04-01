#include "PCH.h"

#include "GameFramework/Public/Assets/SceneImporter.h"

#include "Assets/FbxImporter.h"
#include "Assets/GltfLoader.h"

#include <algorithm>
#include <format>
#include <string>

SceneImportResult SceneImporter::Load(const std::filesystem::path& filePath)
{
	std::string extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});

	if (extension == ".gltf" || extension == ".glb")
	{
		return GltfLoader::Load(filePath);
	}

	if (extension == ".fbx")
	{
		return FbxImporter::Load(filePath);
	}

	SceneImportResult result;
	result.errorMessage = std::format(
	    "SceneImporter: Unsupported asset extension '{}' for '{}'",
	    extension.empty() ? std::string("<none>") : extension,
	    filePath.string());
	return result;
}