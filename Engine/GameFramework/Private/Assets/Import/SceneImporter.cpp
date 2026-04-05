#include "PCH.h"

#include "GameFramework/Public/Assets/Import/SceneImporter.h"

#include "Assets/Importers/FbxImporter.h"
#include "Assets/Importers/GltfLoader.h"
#include "Assets/Import/SceneImportPostProcessor.h"
#include "Core/Public/Paths/PathUtils.h"

#include <format>

SceneImportResult SceneImporter::Load(const std::filesystem::path& filePath)
{
	SceneImportResult result;

	const std::wstring extension = Engine::Paths::GetLowercaseExtension(filePath);

	if (extension == L".gltf" || extension == L".glb")
	{
		result = GltfLoader::Load(filePath);
	}
	else if (extension == L".fbx")
	{
		result = FbxImporter::Load(filePath);
	}
	else
	{
		result.errorMessage = std::format(
		    "SceneImporter: Unsupported asset extension '{}' for '{}'",
		    extension.empty() ? std::string("<none>") : std::string(extension.begin(), extension.end()),
		    filePath.string());
	}

	if (result.bSuccess)
	{
		SceneImportPostProcessor::Finalize(result);
	}

	return result;
}