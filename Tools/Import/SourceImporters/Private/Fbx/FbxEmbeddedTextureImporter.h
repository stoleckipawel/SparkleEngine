#pragma once

#include <assimp/scene.h>

#include <filesystem>
#include <vector>

class FbxEmbeddedTextureImporter final
{
  public:
	static std::vector<std::filesystem::path> ExtractTextures(const aiScene& scene);
};
