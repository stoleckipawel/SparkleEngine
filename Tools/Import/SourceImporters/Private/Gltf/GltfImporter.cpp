#include "PCH.h"

#include "Gltf/GltfImporter.h"

#include "Gltf/GltfCameraImporter.h"
#include "Gltf/GltfAnimationImporter.h"
#include "Gltf/GltfSceneReader.h"
#include "Gltf/GltfGeometryImporter.h"
#include "Gltf/GltfLightImporter.h"
#include "Gltf/GltfMaterialImporter.h"
#include "Gltf/GltfMaterialVariantImporter.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

std::string_view GltfImporter::GetImporterId() const noexcept
{
	return "GltfImporter";
}

bool GltfImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".gltf" || extension == L".glb";
}

SourceImportResult GltfImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.report.sourcePath = filePath;
	result.report.importerId = std::string(GetImporterId());
	result.scene.importerName = result.report.importerId;
	result.scene.sourcePath = filePath;

	GltfScene scene;
	if (!GltfSceneReader::LoadScene(filePath, scene, result))
	{
		return result;
	}

	GltfCameraImporter::ImportCameras(scene.data, result);
	GltfLightImporter::ImportLights(scene.data, result);

	const std::filesystem::path sourceDirectory = filePath.parent_path();
	result.scene.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, sourceDirectory, result);
	GltfMaterialVariantImporter::ImportMaterialVariants(scene.data, result);

	const std::size_t importedMeshInstanceCount = GltfGeometryImporter::CountImportedMeshInstances(scene.data);
	result.ReserveMeshPrimitives(scene.data->meshes_count);
	result.ReserveMeshInstances(importedMeshInstanceCount);
	result.ReserveMeshInstanceGroups(scene.data->nodes_count);
	GltfGeometryImporter::ImportGeometry(scene.data, result);
	GltfAnimationImporter::ImportAnimations(scene.data, result);

	if (result.scene.meshPrimitives.empty() || result.scene.meshInstances.empty())
	{
		SPDLOG_LOGGER_ERROR(
		    g_gltfImporterLogger,
		    "{}",
		    std::format("GltfImporter: No supported mesh primitives found in '{}'", filePath.string()));
		return result;
	}

	result.succeeded = true;
	return result;
}


