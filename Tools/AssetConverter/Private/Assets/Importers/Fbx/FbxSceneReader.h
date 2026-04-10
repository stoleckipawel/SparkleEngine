#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <assimp/scene.h>

#include <filesystem>

namespace Assimp
{
	class Importer;
}

class FbxSceneReader final
{
  public:
	static bool LoadScene(
	    const std::filesystem::path& filePath,
	    Assimp::Importer& importer,
	    const aiScene*& scene,
	    SceneImportResult& result);

	static void CollectSceneWarnings(const aiScene& scene, SceneImportResult& result);

  private:
	static constexpr unsigned int GetPostProcessFlags() noexcept;
	static void ConfigureImporter(Assimp::Importer& importer);
	static bool ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result);
};