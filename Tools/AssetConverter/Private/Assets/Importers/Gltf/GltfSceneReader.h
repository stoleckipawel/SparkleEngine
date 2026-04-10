#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <filesystem>
#include <string>

struct cgltf_data;
struct cgltf_options;

struct GltfScene
{
	cgltf_data* data = nullptr;

	GltfScene() = default;
	~GltfScene();
	GltfScene(const GltfScene&) = delete;
	GltfScene& operator=(const GltfScene&) = delete;
};

class GltfSceneReader final
{
  public:
	static bool LoadScene(const std::filesystem::path& filePath, GltfScene& scene, SceneImportResult& result);
	static void CollectSceneWarnings(const cgltf_data* data, SceneImportResult& result);

  private:
	static bool ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result);
	static bool ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SceneImportResult& result);
	static bool LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SceneImportResult& result);
	static void ValidateGltf(cgltf_data* data, const std::string& pathStr, SceneImportResult& result);
};