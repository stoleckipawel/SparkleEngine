#include "PCH.h"

#include "Gltf/GltfImporter.h"

#include "Gltf/GltfSceneReader.h"
#include "Gltf/GltfGeometryImporter.h"
#include "Gltf/GltfMaterialImporter.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImportAdapters.Gltf");

bool GltfImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".gltf" || extension == L".glb";
}

SourceImportResult GltfImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.importerType = SourceImporterType::Gltf;

	GltfScene scene;
	if (!GltfSceneReader::LoadScene(filePath, scene, result))
	{
		return result;
	}

	GltfSceneReader::CollectSceneWarnings(scene.data, result);

	const std::filesystem::path sourceDirectory = filePath.parent_path();
	result.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, sourceDirectory, result);

	const std::size_t importedMeshInstanceCount = GltfGeometryImporter::CountImportedMeshInstances(scene.data);
	result.Reserve(importedMeshInstanceCount);
	GltfGeometryImporter::ImportGeometry(scene.data, result);

	if (result.meshes.empty())
	{
		SPDLOG_LOGGER_ERROR(g_gltfImporterLogger, "{}", std::format("GltfImporter: No supported mesh primitives found in '{}'", filePath.string()));
		return result;
	}

	result.bSuccess = true;

	SPDLOG_LOGGER_INFO(
	    g_gltfImporterLogger,
	    "{}",
	    std::format(
	        "GltfImporter: Loaded '{}' - {} meshes, {} materials",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size()));

	return result;
}


