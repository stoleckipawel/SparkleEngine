#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Level/LevelRegistry.h"
#include "Runtime/Level/LevelChangeEvents.h"

#include <string>
#include <string_view>
#include <vector>

class Level;
class Scene;
struct SceneLoadResult;

class SPARKLE_ENGINE_API LevelManager final
{
  public:
	explicit LevelManager(Scene& scene) noexcept;
	~LevelManager() noexcept = default;

	LevelManager(const LevelManager&) = delete;
	LevelManager& operator=(const LevelManager&) = delete;
	LevelManager(LevelManager&&) = delete;
	LevelManager& operator=(LevelManager&&) = delete;

	std::string_view GetActiveLevelName() const noexcept { return m_activeLevelName; }
	bool HasActiveLevel() const noexcept { return !m_activeLevelName.empty(); }
	bool IsLevelChangeInProgress() const noexcept { return m_bLevelChangeInProgress; }
	std::vector<std::string> GetRegisteredLevelNames() const;
	LevelChangeEvents& GetLevelChangeEvents() noexcept { return m_levelChangeEvents; }
	const LevelChangeEvents& GetLevelChangeEvents() const noexcept { return m_levelChangeEvents; }

	void RequestLevelChange(std::string_view requestedLevelName) noexcept;

  private:
	static constexpr std::string_view GetEmptyLevelName() noexcept { return "Empty"; }
	static constexpr std::string_view GetStartupLevelName() noexcept { return "Sponza"; }

	void InitializeStartupLevel() noexcept;
	SceneLoadResult LoadLevelFromUnloadedState(const Level& level) noexcept;
	void ResetCameraFromLoadedLevel() noexcept;
	void ProcessLevelChangeRequest(const Level& requestedLevel) noexcept;

	Scene* m_scene = nullptr;
	LevelRegistry m_levelRegistry;
	LevelChangeEvents m_levelChangeEvents;

	std::string m_activeLevelName;
	bool m_bLevelChangeInProgress = false;
};