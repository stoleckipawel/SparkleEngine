#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Level/LevelChangeEvents.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class LevelAsset;
class LevelRegistry;
class GameWorld;
struct GameWorldLoadResult;
namespace Assets
{
	class SceneAssetManager;
}

class SPARKLE_ENGINE_API LevelManager final
{
  public:
	LevelManager(GameWorld& world, Assets::SceneAssetManager& sceneAssetManager);
	~LevelManager() noexcept;

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
	void CaptureSceneToLevel() noexcept;
	void InitializeStartupLevel() noexcept;
	GameWorldLoadResult LoadLevelFromUnloadedState(const LevelAsset& level) noexcept;
	void ProcessLevelChangeRequest(LevelAsset& requestedLevel) noexcept;

	GameWorld* m_gameWorld = nullptr;
	Assets::SceneAssetManager* m_sceneAssetManager = nullptr;
	std::unique_ptr<LevelRegistry> m_levelRegistry;
	LevelChangeEvents m_levelChangeEvents;
	LevelAsset* m_activeLevel = nullptr;
	LevelAsset* m_pendingLevelChange = nullptr;
	bool m_bLevelChangeInProgress = false;
};
