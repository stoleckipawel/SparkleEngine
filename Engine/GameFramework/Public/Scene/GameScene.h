#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Lighting/GameSceneLightingState.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Mesh;
class GameCamera;
class Level;
struct ImportedMeshRequest;

enum class GameSceneLoadStatus : std::uint8_t
{
	Succeeded = 0,
	Failed
};

struct SPARKLE_ENGINE_API GameSceneLoadResult
{
	GameSceneLoadStatus status = GameSceneLoadStatus::Failed;
	std::string errorMessage;

	bool Succeeded() const noexcept { return status == GameSceneLoadStatus::Succeeded; }
};

class SPARKLE_ENGINE_API GameScene final
{
  public:
	GameScene();
	~GameScene() noexcept;

	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	GameScene(GameScene&&) = delete;
	GameScene& operator=(GameScene&&) = delete;

	GameCamera& GetCamera() noexcept;
	const GameCamera& GetCamera() const noexcept;
	GameSceneLightingState& GetLightingState() noexcept { return m_lightingState; }
	const GameSceneLightingState& GetLightingState() const noexcept { return m_lightingState; }

	GameSceneLoadResult LoadLevel(const Level& level);
	GameSceneLoadResult LoadLevel(const LevelDesc& desc);

	void Clear();

	bool LoadGltf(const std::filesystem::path& assetPath);

	const std::vector<MaterialDesc>& GetLoadedMaterials() const noexcept { return m_loadedMaterials; }

	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return m_meshes; }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

  private:
	bool LoadImportedMeshRequests(const LevelDesc& desc, std::string& errorMessage);
	bool LoadImportedMeshRequest(const ImportedMeshRequest& request, std::string& errorMessage);
	bool AppendResolvedGltf(const std::filesystem::path& resolvedPath);

	std::unique_ptr<GameCamera> m_gameCamera;
	GameSceneLightingState m_lightingState;

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<MaterialDesc> m_loadedMaterials;
};
