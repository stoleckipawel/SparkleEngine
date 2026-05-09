#pragma once

#include "SourceImportResult.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <filesystem>

class FbxSceneReader final
{
  public:
	static bool LoadScene(
	    const std::filesystem::path& filePath,
	    Assimp::Importer& importer,
	    const aiScene*& scene,
	    SourceImportResult& result);

	static void CollectSceneWarnings(const aiScene& scene, SourceImportResult& result);

  private:
	static constexpr unsigned int GetPostProcessFlags() noexcept;
	static void ConfigureImporter(Assimp::Importer& importer);
	static bool ValidateInputPath(const std::filesystem::path& filePath, SourceImportResult& result);
};


