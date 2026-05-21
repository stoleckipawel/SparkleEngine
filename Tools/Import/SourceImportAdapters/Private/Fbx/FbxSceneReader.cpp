#include "PCH.h"

#include "Fbx/FbxSceneReader.h"

#include "Diagnostics/FbxImportDiagnosticLog.h"
#include "Diagnostics/FbxSceneDiagnostics.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

constexpr unsigned int FbxSceneReader::GetPostProcessFlags() noexcept
{
	return aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
	       aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_ImproveCacheLocality | aiProcess_ConvertToLeftHanded;
}

void FbxSceneReader::ConfigureImporter(Assimp::Importer& importer)
{
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

bool FbxSceneReader::ValidateInputPath(const std::filesystem::path& filePath, SourceImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	FbxImportDiagnosticLog::ReportMissingFile(filePath, result);
	return false;
}

bool FbxSceneReader::LoadScene(
	const std::filesystem::path& filePath,
	Assimp::Importer& importer,
	const aiScene*& scene,
	SourceImportResult& result)
{
	if (!ValidateInputPath(filePath, result))
	{
		return false;
	}

	ConfigureImporter(importer);
	scene = importer.ReadFile(filePath.string(), GetPostProcessFlags());
    if (scene != nullptr && scene->mRootNode != nullptr)
	{
		return true;
	}

	FbxImportDiagnosticLog::ReportParseFailure(filePath, importer.GetErrorString(), result);
	return false;
}

void FbxSceneReader::CollectSceneWarnings(const aiScene& scene, SourceImportResult& result)
{
	const SourceSceneFeatureDiagnostics sceneFeatures = FbxSceneDiagnostics::CaptureFeatures(scene);
	SourceImportDiagnosticsRecorder::RecordSceneFeatures(result, sceneFeatures);

	if (scene.HasAnimations())
	{
		FbxImportDiagnosticLog::ReportIgnoredAnimations(sceneFeatures.animationCount, result);
	}

	if (scene.HasTextures())
	{
		FbxImportDiagnosticLog::ReportIgnoredEmbeddedTextures(sceneFeatures.embeddedTextureCount, result);
	}

	if (scene.HasCameras())
	{
		FbxImportDiagnosticLog::ReportIgnoredCameras(sceneFeatures.cameraNodeCount, result);
	}

	if (scene.HasLights())
	{
		FbxImportDiagnosticLog::ReportIgnoredLights(sceneFeatures.lightNodeCount, result);
	}
}


