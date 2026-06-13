#include "PCH.h"

#include "Gltf/GltfImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/GltfGeometryInstancingDiagnostics.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "Gltf/GltfCameraImporter.h"
#include "Gltf/GltfAnimationImporter.h"
#include "Gltf/GltfImportFeatureDiagnostics.h"
#include "Gltf/GltfSceneReader.h"
#include "Gltf/GltfGeometryImporter.h"
#include "Gltf/GltfLightImporter.h"
#include "Gltf/GltfMaterialImporter.h"
#include "Gltf/GltfMaterialVariantImporter.h"
#include "Gltf/GltfMorphImportDiagnostics.h"
#include "Gltf/GltfSkinImportDiagnostics.h"

#include <cgltf.h>

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

	GltfSceneReader::CollectSceneWarnings(scene.data, result);
	GltfCameraImporter::ImportCameras(scene.data, result);
	GltfLightImporter::ImportLights(scene.data, result);
	GltfImportFeatureDiagnostics::RecordImportedFeatureSupport(result);

	const std::filesystem::path sourceDirectory = filePath.parent_path();
	SourceImportDiagnosticsRecorder::RecordSourceSummary(result, scene.data->meshes_count, scene.data->materials_count);
	result.scene.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, sourceDirectory, result);
	GltfMaterialVariantImporter::ImportMaterialVariants(scene.data, result);

	SourceImportDiagnosticsRecorder::RecordGeometryInstancingBaseline(result, GltfGeometryInstancingDiagnostics::CaptureBaseline(scene.data));
	const std::size_t importedMeshInstanceCount = GltfGeometryImporter::CountImportedMeshInstances(scene.data);
	result.ReserveMeshPrimitives(result.diagnostics.geometryInstancing.uniqueMeshPrimitiveCandidateCount);
	result.ReserveMeshInstances(importedMeshInstanceCount);
	result.ReserveMeshInstanceGroups(result.diagnostics.geometryInstancing.authoredInstanceGroupCount);
	GltfGeometryImporter::ImportGeometry(scene.data, result);
	GltfAnimationImporter::ImportAnimations(scene.data, result);
	GltfSkinImportDiagnostics::ReportStaticOnlySkinnedNodes(result);
	GltfMorphImportDiagnostics::ReportUnsupportedWeightedNodes(result);
	GltfGeometryInstancingDiagnostics::RecordImportedPlacements(result);
	SourceImportDiagnosticsRecorder::RecordImportedScenePayload(result);

	if (result.scene.meshPrimitives.empty() || result.scene.meshInstances.empty())
	{
		GltfImportDiagnosticLog::ReportNoSupportedMeshPrimitives(filePath, result);
		return result;
	}

	result.succeeded = true;
	GltfImportDiagnosticLog::ReportLoadedScene(filePath, result);

	return result;
}


