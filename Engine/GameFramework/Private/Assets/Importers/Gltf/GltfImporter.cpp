#include "PCH.h"

#include "Assets/Importers/Gltf/GltfImporter.h"

#include "Assets/Importers/Gltf/GltfSceneReader.h"
#include "Assets/Importers/Gltf/GltfGeometryImporter.h"
#include "Assets/Importers/Gltf/GltfMaterialImporter.h"

#include <cgltf.h>

#include <format>

bool GltfImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".gltf" || extension == L".glb";
}

SceneImportResult GltfImporter::Import(const std::filesystem::path& filePath) const
{
	SceneImportResult result;
	result.importerType = SceneImporterType::Gltf;

	GltfScene scene;
	if (!GltfSceneReader::LoadScene(filePath, scene, result))
	{
		return result;
	}

	GltfSceneReader::CollectSceneWarnings(scene.data, result);

	result.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, filePath.parent_path(), result);

	const std::size_t totalPrimitives = GltfGeometryImporter::CountTotalPrimitives(scene.data);
	result.Reserve(totalPrimitives);
	GltfGeometryImporter::ImportGeometry(scene.data, result);

	if (result.meshes.empty())
	{
		LOG_ERROR(std::format("GltfImporter: No supported mesh primitives found in '{}'", filePath.string()));
		return result;
	}

	result.bSuccess = true;

	LOG_INFO(
	    std::format(
	        "GltfImporter: Loaded '{}' — {} meshes, {} materials",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size()));

	return result;
}