#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

#include <cstddef>
#include <filesystem>
#include <vector>

class SPARKLE_ENGINE_API SceneTextures final
{
  public:
	SceneTextures() noexcept = default;
	~SceneTextures() noexcept = default;

	SceneTextures(const SceneTextures&) = delete;
	SceneTextures& operator=(const SceneTextures&) = delete;
	SceneTextures(SceneTextures&&) = delete;
	SceneTextures& operator=(SceneTextures&&) = delete;

	std::size_t GetTextureCount() const noexcept { return m_texturePaths.size(); }
	const std::filesystem::path& GetTexturePath(std::size_t index) const noexcept { return m_texturePaths[index]; }

	void AppendMaterialTextureReferences(const std::vector<MaterialDesc>& materialDescs);
	void AppendTexturePaths(const std::vector<std::filesystem::path>& texturePaths);
	TextureSnapshot CaptureSnapshot() const;
	void Reset() noexcept { m_texturePaths.clear(); }

  private:
	std::vector<std::filesystem::path> m_texturePaths;
};