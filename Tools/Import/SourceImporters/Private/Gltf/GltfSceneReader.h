#pragma once

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
	static void LoadScene(const std::filesystem::path& filePath, GltfScene& scene);

  private:
	static void ValidateInputPath(const std::filesystem::path& filePath);
	static void ParseGltfFile(cgltf_options& options, const std::string& path, cgltf_data*& outData);
	static void LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& path);
	static void ValidateGltf(cgltf_data* data, const std::string& path);
};
