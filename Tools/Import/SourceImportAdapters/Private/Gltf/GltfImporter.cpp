#include "PCH.h"

#include "Gltf/GltfImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/GltfGeometryInstancingDiagnostics.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "Gltf/GltfCameraImporter.h"
#include "Gltf/GltfSceneReader.h"
#include "Gltf/GltfGeometryImporter.h"
#include "Gltf/GltfLightImporter.h"
#include "Gltf/GltfMaterialImporter.h"

#include <cgltf.h>

#include <algorithm>

namespace
{
	void RecordLightImportDiagnostics(SourceImportResult& result)
	{
		if (result.scene.lights.empty())
		{
			return;
		}

		const auto isPointLight = [](const ImportedLight& light) noexcept { return light.kind == ImportedLightKind::Point; };
		const auto isSpotLight = [](const ImportedLight& light) noexcept { return light.kind == ImportedLightKind::Spot; };
		const auto isUnsupportedRuntimeLight = [](const ImportedLight& light) noexcept {
			return light.kind == ImportedLightKind::Point || light.kind == ImportedLightKind::Spot || light.kind == ImportedLightKind::Unknown;
		};

		const std::size_t pointLightCount = static_cast<std::size_t>(
		    std::count_if(result.scene.lights.begin(), result.scene.lights.end(), isPointLight));
		const std::size_t spotLightCount = static_cast<std::size_t>(
		    std::count_if(result.scene.lights.begin(), result.scene.lights.end(), isSpotLight));
		const bool hasUnsupportedRuntimeLights =
		    std::any_of(result.scene.lights.begin(), result.scene.lights.end(), isUnsupportedRuntimeLight);

		result.diagnostics.featureCapabilities.lightNodes = {
		    result.scene.lights.size(),
		    hasUnsupportedRuntimeLights ? SourceImportFeatureSupport::PartiallyImported : SourceImportFeatureSupport::Imported};

		if (pointLightCount > 0)
		{
			GltfImportDiagnosticLog::ReportUnsupportedPointLights(pointLightCount, result);
		}

		if (spotLightCount > 0)
		{
			GltfImportDiagnosticLog::ReportUnsupportedSpotLights(spotLightCount, result);
		}
	}
}  // namespace

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
	GltfCameraImporter::ImportCameras(scene.data, result);
	GltfLightImporter::ImportLights(scene.data, result);
	if (!result.scene.cameras.empty())
	{
		result.diagnostics.featureCapabilities.cameraNodes = {
		    result.scene.cameras.size(),
		    SourceImportFeatureSupport::Imported};
	}
	RecordLightImportDiagnostics(result);

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


