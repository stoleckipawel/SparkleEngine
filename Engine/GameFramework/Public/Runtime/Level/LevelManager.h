#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Level/LevelRegistry.h"
#include "Runtime/Level/LevelChangeEvents.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class Level;
class GameScene;
class GameCameraController;
class GameSceneLightingState;
struct GameSceneLoadResult;

class SPARKLE_ENGINE_API LevelManager final
{
  public:
	explicit LevelManager(GameScene& scene) noexcept;
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
	void RegisterGameCameraController(GameCameraController& gameCameraController) noexcept;
	GameCameraController* GetGameCameraController() noexcept { return m_gameCameraController; }
	const GameCameraController* GetGameCameraController() const noexcept { return m_gameCameraController; }
	Level* GetActiveLevel() noexcept { return m_activeLevel; }
	const Level* GetActiveLevel() const noexcept { return m_activeLevel; }
	GameSceneLightingState* GetGameSceneLightingState() noexcept;
	const GameSceneLightingState* GetGameSceneLightingState() const noexcept;
	bool SaveActiveLevel() noexcept;

  private:
	static constexpr std::string_view GetEmptyLevelName() noexcept { return "Empty"; }
	static constexpr std::string_view GetStartupLevelName() noexcept { return "Sponza"; }

	void InitializeActiveLevel() noexcept;
	void InitializeStartupLevel() noexcept;
	GameSceneLoadResult LoadLevelFromUnloadedState(const Level& level) noexcept;
	void ProcessLevelChangeRequest(Level& requestedLevel) noexcept;

	GameScene* m_gameScene = nullptr;
	GameCameraController* m_gameCameraController = nullptr;
	LevelRegistry m_levelRegistry;
	LevelChangeEvents m_levelChangeEvents;
	Level* m_activeLevel = nullptr;
	bool m_bLevelChangeInProgress = false;
};