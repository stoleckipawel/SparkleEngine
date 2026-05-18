#include "PCH.h"

#include "Fbx/FbxImporter.h"

#include "Diagnostics/FbxImportDiagnosticLog.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "Fbx/FbxGeometryImporter.h"
#include "Fbx/FbxMaterialImporter.h"
#include "Fbx/FbxSceneReader.h"

#include <assimp/Importer.hpp>

bool FbxImporter::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".fbx";
}

SourceImportResult FbxImporter::Import(const std::filesystem::path& filePath) const
{
	SourceImportResult result;
	result.scene.importerType = SourceImporterType::Fbx;
	result.scene.sourcePath = filePath;

	Assimp::Importer importer;
	const aiScene* scene = nullptr;
	if (!FbxSceneReader::LoadScene(filePath, importer, scene, result))
	{
		return result;
	}

	SourceImportDiagnosticsRecorder::RecordSourceSummary(result, scene->mNumMeshes, scene->mNumMaterials);
	result.scene.materials.reserve(scene->mNumMaterials);
	const std::size_t importedMeshInstanceCount = FbxGeometryImporter::CountImportedMeshInstances(*scene->mRootNode);
	SourceImportDiagnosticsRecorder::RecordGeometryInstancingPrimitiveCandidates(result, scene->mNumMeshes);
	result.ReserveMeshPrimitives(scene->mNumMeshes);
	result.ReserveMeshInstances(importedMeshInstanceCount);

	FbxSceneReader::CollectSceneWarnings(*scene, result);
	FbxMaterialImporter::ImportMaterials(*scene, filePath.parent_path(), result);
	FbxGeometryImporter::ImportGeometry(*scene, result);
	SourceImportDiagnosticsRecorder::RecordGeometryInstancingPlacements(result);
	SourceImportDiagnosticsRecorder::RecordImportedScenePayload(result);

	if (result.scene.meshPrimitives.empty() || result.scene.meshInstances.empty())
	{
		FbxImportDiagnosticLog::ReportNoSupportedStaticMeshes(filePath, result);
		return result;
	}

	result.succeeded = true;
	FbxImportDiagnosticLog::ReportLoadedScene(filePath, result);

	return result;
}


