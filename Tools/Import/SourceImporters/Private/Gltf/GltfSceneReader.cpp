#define CGLTF_IMPLEMENTATION
#include "PCH.h"

#include "Gltf/GltfSceneReader.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfSceneReaderLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

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

	(void)result;
	SPDLOG_LOGGER_ERROR(g_gltfSceneReaderLogger, "{}", std::format("GltfImporter: File not found: {}", filePath.string()));
	return false;
}

bool GltfSceneReader::ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SourceImportResult& result)
{
	const cgltf_result parseResult = cgltf_parse_file(&options, pathStr.c_str(), &outData);
	if (parseResult == cgltf_result_success)
	{
		return true;
	}

	(void)result;
	SPDLOG_LOGGER_ERROR(
	    g_gltfSceneReaderLogger,
	    "{}",
	    std::format("GltfImporter: Failed to parse '{}' (cgltf error {})", pathStr, static_cast<int>(parseResult)));
	return false;
}

bool GltfSceneReader::LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SourceImportResult& result)
{
	const cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathStr.c_str());
	if (bufferResult == cgltf_result_success)
	{
		return true;
	}

	(void)result;
	SPDLOG_LOGGER_ERROR(
	    g_gltfSceneReaderLogger,
	    "{}",
	    std::format("GltfImporter: Failed to load buffers for '{}' (cgltf error {})", pathStr, static_cast<int>(bufferResult)));
	return false;
}

void GltfSceneReader::ValidateGltf(cgltf_data* data, const std::string& pathStr, SourceImportResult& result)
{
	const cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		(void)result;
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format("GltfImporter: Validation warnings for '{}' (cgltf error {})", pathStr, static_cast<int>(validateResult)));
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

