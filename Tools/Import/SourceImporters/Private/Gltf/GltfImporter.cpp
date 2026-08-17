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

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/WorldCoordinateSystem.h"

std::string_view GltfImporter::GetImporterId() const noexcept
{
	return "GltfImporter";
}

bool GltfImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".gltf" || extension == L".glb";
}

SourceImportOutput GltfImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportOutput output;
	output.provenance.sourcePath = filePath;
	output.provenance.importerId = std::string(GetImporterId());
	output.provenance.sourceMetersPerUnit = 1.0f;
	output.scene.coordinateContractVersion = WorldCoordinates::kCoordinateContractVersion;

	GltfScene scene;
	GltfSceneReader::LoadScene(filePath, scene);

	GltfCameraImporter::ImportCameras(scene.data, output);
	GltfLightImporter::ImportLights(scene.data, output);

	const std::filesystem::path sourceDirectory = filePath.parent_path();
	output.scene.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, sourceDirectory, output);
	GltfMaterialVariantImporter::ImportMaterialVariants(scene.data, output);

	const std::size_t importedMeshInstanceCount = GltfGeometryImporter::CountImportedMeshInstances(scene.data);
	output.ReserveMeshPrimitives(scene.data->meshes_count);
	output.ReserveMeshInstances(importedMeshInstanceCount);
	output.ReserveMeshInstanceGroups(scene.data->nodes_count);
	GltfGeometryImporter::ImportGeometry(scene.data, output);
	GltfAnimationImporter::ImportAnimations(scene.data, output);

	if (output.scene.meshPrimitives.empty() != output.scene.meshInstances.empty()
	    || (output.scene.meshPrimitives.empty() && output.scene.cameras.empty() && output.scene.lights.empty()
	        && output.scene.animations.empty()))
	{
		throw Diagnostics::Error("glTF import produced incomplete mesh content or no supported scene content.");
	}

	return output;
}
