#define CGLTF_IMPLEMENTATION
#include "PCH.h"

#include "Assets/Importers/Gltf/GltfSceneReader.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfSceneReaderLogger = Logging::GetOrCreateLogger("Tools.AssetConverter.Gltf");

GltfScene::~GltfScene()
{
	cgltf_free(data);
}

bool GltfSceneReader::ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	SPDLOG_LOGGER_ERROR(g_gltfSceneReaderLogger, "{}", std::format("GltfImporter: File not found: {}", filePath.string()));
	return false;
}

bool GltfSceneReader::ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SceneImportResult& result)
{
	const cgltf_result parseResult = cgltf_parse_file(&options, pathStr.c_str(), &outData);
	if (parseResult == cgltf_result_success)
	{
		return true;
	}

	SPDLOG_LOGGER_ERROR(
	    g_gltfSceneReaderLogger,
	    "{}",
	    std::format("GltfImporter: Failed to parse '{}' (cgltf error {})", pathStr, static_cast<int>(parseResult)));
	return false;
}

bool GltfSceneReader::LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SceneImportResult& result)
{
	const cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathStr.c_str());
	if (bufferResult == cgltf_result_success)
	{
		return true;
	}

	SPDLOG_LOGGER_ERROR(
	    g_gltfSceneReaderLogger,
	    "{}",
	    std::format("GltfImporter: Failed to load buffers for '{}' (cgltf error {})", pathStr, static_cast<int>(bufferResult)));
	return false;
}

void GltfSceneReader::ValidateGltf(cgltf_data* data, const std::string& pathStr, SceneImportResult& result)
{
	const cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format("GltfImporter: Validation warnings for '{}' (cgltf error {})", pathStr, static_cast<int>(validateResult)));
	}
}

bool GltfSceneReader::LoadScene(const std::filesystem::path& filePath, GltfScene& scene, SceneImportResult& result)
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

void GltfSceneReader::CollectSceneWarnings(const cgltf_data* data, SceneImportResult& result)
{
	if (data->animations_count > 0)
	{
		SPDLOG_LOGGER_WARN(g_gltfSceneReaderLogger, "{}", std::format("GltfImporter: {} animations are present and will be ignored", data->animations_count));
	}

	if (data->variants_count > 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format("GltfImporter: {} material variants are present and will be ignored", data->variants_count));
	}

	std::size_t cameraNodeCount = 0;
	std::size_t lightNodeCount = 0;
	std::size_t skinnedNodeCount = 0;
	std::size_t weightedNodeCount = 0;
	std::size_t instancedNodeCount = 0;

	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		cameraNodeCount += node.camera != nullptr ? 1u : 0u;
		lightNodeCount += node.light != nullptr ? 1u : 0u;
		skinnedNodeCount += node.skin != nullptr ? 1u : 0u;
		weightedNodeCount += node.weights_count > 0 ? 1u : 0u;
		instancedNodeCount += node.has_mesh_gpu_instancing ? 1u : 0u;
	}

	if (cameraNodeCount > 0)
	{
		SPDLOG_LOGGER_WARN(g_gltfSceneReaderLogger, "{}", std::format("GltfImporter: {} nodes contain cameras and they will be ignored", cameraNodeCount));
	}

	if (lightNodeCount > 0)
	{
		SPDLOG_LOGGER_WARN(g_gltfSceneReaderLogger, "{}", std::format("GltfImporter: {} nodes contain lights and they will be ignored", lightNodeCount));
	}

	if (skinnedNodeCount > 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format("GltfImporter: {} skinned nodes are present and will be imported as static data only", skinnedNodeCount));
	}

	if (weightedNodeCount > 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format("GltfImporter: {} weighted nodes are present and morph weights will be ignored", weightedNodeCount));
	}

	if (instancedNodeCount > 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfSceneReaderLogger,
		    "{}",
		    std::format(
		        "GltfImporter: {} nodes use mesh GPU instancing and will be flattened to regular mesh instances",
		        instancedNodeCount));
	}
}