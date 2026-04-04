#include "PCH.h"

#include "GameFramework/Public/Assets/Import/SceneImporter.h"

#include "Assets/Importers/FbxImporter.h"
#include "Assets/Importers/GltfLoader.h"
#include "Assets/Import/SceneImportPostProcessor.h"
#include "Core/Public/Strings/StringUtils.h"

#include <format>
#include <string>

SceneImportResult SceneImporter::Load(const std::filesystem::path& filePath)
{
	SceneImportResult result;

	std::string extension = Engine::Strings::ToLowerCopy(filePath.extension().string());

	if (extension == ".gltf" || extension == ".glb")
	{
		result = GltfLoader::Load(filePath);
	}
	else if (extension == ".fbx")
	{
		result = FbxImporter::Load(filePath);
	}
	else
	{
		result.errorMessage = std::format(
		    "SceneImporter: Unsupported asset extension '{}' for '{}'",
		    extension.empty() ? std::string("<none>") : extension,
		    filePath.string());
	}

	if (result.bSuccess)
	{
		SceneImportPostProcessor::Finalize(result);
	}

	return result;
}