#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Level/LevelRegistry.h"
#include "Level/LevelChangeEvents.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class LevelAsset;
class GameScene;
struct GameSceneLoadResult;
namespace Assets
{
	class SceneAssetManager;
}

class SPARKLE_ENGINE_API LevelManager final
{
  public:
	LevelManager(GameScene& scene, Assets::SceneAssetManager& sceneAssetManager) noexcept;
	~LevelManager() noexcept = default;

	LevelManager(const LevelManager&) = delete;
	LevelManager& operator=(const LevelManager&) = delete;
	LevelManager(LevelManager&&) = delete;
	LevelManager& operator=(LevelManager&&) = delete;

	bool HasActiveLevel() const noexcept { return m_activeLevel != nullptr; }
	bool IsLevelChangeInProgress() const noexcept { return m_bLevelChangeInProgress; }
	std::vector<std::string> GetRegisteredLevelNames() const;
	LevelChangeEvents& GetLevelChangeEvents() noexcept { return m_levelChangeEvents; }
	const LevelChangeEvents& GetLevelChangeEvents() const noexcept { return m_levelChangeEvents; }

	void RequestLevelChange(std::string_view requestedLevelName) noexcept;
	void ProcessPendingLevelChange() noexcept;

	LevelAsset* GetActiveLevel() noexcept { return m_activeLevel; }
	const LevelAsset* GetActiveLevel() const noexcept { return m_activeLevel; }
	bool SaveActiveLevel() noexcept;

  private:
	static constexpr std::string_view GetEmptyLevelName() noexcept { return "Empty"; }
	static constexpr std::string_view GetStartupLevelName() noexcept { return "Sponza"; }

	void ApplyLevelToScene() noexcept;
	void CaptureSceneToLevel() noexcept;
	void InitializeStartupLevel() noexcept;
	GameSceneLoadResult LoadLevelFromUnloadedState(const LevelAsset& level) noexcept;
	void ProcessLevelChangeRequest(LevelAsset& requestedLevel) noexcept;

	GameScene* m_gameScene = nullptr;
	Assets::SceneAssetManager* m_sceneAssetManager = nullptr;
	LevelRegistry m_levelRegistry;
	LevelChangeEvents m_levelChangeEvents;
	LevelAsset* m_activeLevel = nullptr;
	LevelAsset* m_pendingLevelChange = nullptr;
	bool m_bLevelChangeInProgress = false;
};