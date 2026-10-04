#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <filesystem>

class AssimpSceneReader final
{
public:
	static const aiScene& LoadScene(const std::filesystem::path& filePath, Assimp::Importer& importer);
	static float GetFbxMetersPerSourceUnit(const Assimp::Importer& importer);

private:
	static constexpr unsigned int GetPostProcessFlags() noexcept;
	static void ValidateInputPath(const std::filesystem::path& filePath);
};
