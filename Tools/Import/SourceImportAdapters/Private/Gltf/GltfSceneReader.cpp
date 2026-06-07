#define CGLTF_IMPLEMENTATION
#include "PCH.h"

#include "Gltf/GltfSceneReader.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/GltfSceneDiagnostics.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <cgltf.h>

GltfScene::~GltfScene()
{
	cgltf_free(data);
}

bool GltfSceneReader::ValidateInputPath(const std::filesystem::path& filePath, SourceImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	GltfImportDiagnosticLog::ReportMissingFile(filePath, result);
	return false;
}

bool GltfSceneReader::ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SourceImportResult& result)
{
	const cgltf_result parseResult = cgltf_parse_file(&options, pathStr.c_str(), &outData);
	if (parseResult == cgltf_result_success)
	{
		return true;
	}

	GltfImportDiagnosticLog::ReportParseFailure(pathStr, static_cast<int>(parseResult), result);
	return false;
}

bool GltfSceneReader::LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SourceImportResult& result)
{
	const cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathStr.c_str());
	if (bufferResult == cgltf_result_success)
	{
		return true;
	}

	GltfImportDiagnosticLog::ReportBufferLoadFailure(pathStr, static_cast<int>(bufferResult), result);
	return false;
}

void GltfSceneReader::ValidateGltf(cgltf_data* data, const std::string& pathStr, SourceImportResult& result)
{
	const cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		GltfImportDiagnosticLog::ReportValidationWarning(pathStr, static_cast<int>(validateResult), result);
	}
}

bool GltfSceneReader::LoadScene(const std::filesystem::path& filePath, GltfScene& scene, SourceImportResult& result)
{
	if (!ValidateInputPath(filePath, result))
	{
		return false;
	}

	const std::string pathStr = filePath.string();
	cgltf_options options{};
	if (!ParseGltfFile(options, pathStr, scene.data, result))
	{
		return false;
	}

	if (!LoadGltfBuffers(options, scene.data, pathStr, result))
	{
		return false;
	}

	ValidateGltf(scene.data, pathStr, result);
	return true;
}

void GltfSceneReader::CollectSceneWarnings(const cgltf_data* data, SourceImportResult& result)
{
	const SourceSceneFeatureDiagnostics sceneFeatures = GltfSceneDiagnostics::CaptureFeatures(data);
	SourceImportDiagnosticsRecorder::RecordSceneFeatures(result, sceneFeatures);

	if (sceneFeatures.materialVariantCount > 0)
	{
		GltfImportDiagnosticLog::ReportIgnoredMaterialVariants(sceneFeatures.materialVariantCount, result);
	}

}


