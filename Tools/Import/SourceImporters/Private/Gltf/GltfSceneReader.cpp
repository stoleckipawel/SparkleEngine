#define CGLTF_IMPLEMENTATION
#include "PCH.h"

#include "Gltf/GltfSceneReader.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <format>

GltfScene::~GltfScene()
{
	cgltf_free(data);
}

void GltfSceneReader::ValidateInputPath(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		throw Diagnostics::Error(std::format("glTF source file does not exist: '{}'.", filePath.string()));
	}
}

void GltfSceneReader::ParseGltfFile(cgltf_options& options, const std::string& path, cgltf_data*& outData)
{
	const cgltf_result parseResult = cgltf_parse_file(&options, path.c_str(), &outData);
	if (parseResult != cgltf_result_success)
	{
		throw Diagnostics::Error(std::format("Cannot parse glTF source '{}' (cgltf error {}).", path, static_cast<int>(parseResult)));
	}
}

void GltfSceneReader::LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& path)
{
	const cgltf_result bufferResult = cgltf_load_buffers(&options, data, path.c_str());
	if (bufferResult != cgltf_result_success)
	{
		throw Diagnostics::Error(std::format("Cannot load glTF buffers for '{}' (cgltf error {}).", path, static_cast<int>(bufferResult)));
	}
}

void GltfSceneReader::ValidateGltf(cgltf_data* data, const std::string& path)
{
	const cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		throw Diagnostics::Error(
		    std::format("glTF source '{}' fails cgltf validation (error {}).", path, static_cast<int>(validateResult)));
	}
}

void GltfSceneReader::LoadScene(const std::filesystem::path& filePath, GltfScene& scene)
{
	ValidateInputPath(filePath);

	const std::string path = filePath.string();
	cgltf_options options{};
	ParseGltfFile(options, path, scene.data);
	LoadGltfBuffers(options, scene.data, path);
	ValidateGltf(scene.data, path);
}
