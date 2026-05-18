#include "PCH.h"

#include "Gltf/GltfImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/GltfGeometryInstancingDiagnostics.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "Gltf/GltfSceneReader.h"
#include "Gltf/GltfGeometryImporter.h"
#include "Gltf/GltfMaterialImporter.h"

#include <cgltf.h>

bool GltfImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".gltf" || extension == L".glb";
}

SourceImportResult GltfImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.scene.importerName = "GltfImporter";
	result.scene.sourcePath = filePath;

	GltfScene scene;
	if (!GltfSceneReader::LoadScene(filePath, scene, result))
	{
		return result;
	}

	GltfSceneReader::CollectSceneWarnings(scene.data, result);

	const std::filesystem::path sourceDirectory = filePath.parent_path();
	SourceImportDiagnosticsRecorder::RecordSourceSummary(result, scene.data->meshes_count, scene.data->materials_count);
	result.scene.materials.reserve(scene.data->materials_count);
	GltfMaterialImporter::ImportMaterials(scene.data, sourceDirectory, result);

	SourceImportDiagnosticsRecorder::RecordGeometryInstancingBaseline(result, GltfGeometryInstancingDiagnostics::CaptureBaseline(scene.data));
	const std::size_t importedMeshInstanceCount = GltfGeometryImporter::CountImportedMeshInstances(scene.data);
	result.ReserveMeshPrimitives(result.diagnostics.geometryInstancing.uniqueMeshPrimitiveCandidateCount);
	result.ReserveMeshInstances(importedMeshInstanceCount);
	result.ReserveMeshInstanceGroups(result.diagnostics.geometryInstancing.authoredInstanceGroupCount);
	GltfGeometryImporter::ImportGeometry(scene.data, result);
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


