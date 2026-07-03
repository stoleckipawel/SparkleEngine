#pragma once

#include "SourceImportResult.h"

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
	static bool LoadScene(const std::filesystem::path& filePath, GltfScene& scene, SourceImportResult& result);

  private:
	static bool ValidateInputPath(const std::filesystem::path& filePath, SourceImportResult& result);
	static bool ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, SourceImportResult& result);
	static bool LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, SourceImportResult& result);
	static void ValidateGltf(cgltf_data* data, const std::string& pathStr, SourceImportResult& result);
};


