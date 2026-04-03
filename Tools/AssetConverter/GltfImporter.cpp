#include "GltfImporter.h"

#include "GameFramework/Public/Assets/GltfLoader.h"

SceneImportResult GltfImporter::Load(const std::filesystem::path& filePath)
{
	return GltfLoader::Load(filePath);
}
