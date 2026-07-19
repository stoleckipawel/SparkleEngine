#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Level/LevelChangeEvents.h"
#include "Level/LevelLoadOperation.h"

#include <memory>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class LevelAsset;
class LevelRegistry;
class GameWorld;
namespace Assets
{
	class SceneLoadExecutionService;
}
class TaskExecutor;
class TaskScope;

class SPARKLE_ENGINE_API LevelManager final
{
  public:
	LevelManager(
	    GameWorld& world,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationScope);
	~LevelManager() noexcept;

	LevelManager(const LevelManager&) = delete;
	LevelManager& operator=(const LevelManager&) = delete;
	LevelManager(LevelManager&&) = delete;
	LevelManager& operator=(LevelManager&&) = delete;

	bool HasActiveLevel() const noexcept { return m_activeLevel != nullptr; }
	bool IsLevelChangeInProgress() const noexcept { return m_levelChangeInProgress; }
	std::vector<std::string> GetRegisteredLevelNames() const;
	LevelChangeEvents& GetLevelChangeEvents() noexcept { return m_levelChangeEvents; }
	const LevelChangeEvents& GetLevelChangeEvents() const noexcept { return m_levelChangeEvents; }
	LevelLoadOperationProgress GetLoadProgress() const noexcept;
	std::string_view GetLastLoadDiagnostic() const noexcept { return m_lastLoadDiagnostic; }

	void RequestLevelChange(std::string_view requestedLevelName) noexcept;
	void ProcessPendingLevelChange() noexcept;

	LevelAsset* GetActiveLevel() noexcept { return m_activeLevel; }
	const LevelAsset* GetActiveLevel() const noexcept { return m_activeLevel; }
	bool SaveActiveLevel() noexcept;

  private:
	void CaptureSceneToLevel() noexcept;
	void InitializeStartupLevel() noexcept;
	void StartLevelChange(LevelAsset& requestedLevel) noexcept;
	void CompleteLevelChange() noexcept;

	GameWorld* m_gameWorld = nullptr;
	std::unique_ptr<LevelRegistry> m_levelRegistry;
	std::unique_ptr<Assets::SceneLoadExecutionService> m_loadExecution;
	LevelChangeEvents m_levelChangeEvents;
	LevelAsset* m_activeLevel = nullptr;
	LevelAsset* m_pendingLevelChange = nullptr;
	LevelAsset* m_loadingLevel = nullptr;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_latestRequestId = 0;
	std::uint64_t m_documentGeneration = 1;
	std::string m_lastLoadDiagnostic;
	bool m_levelChangeInProgress = false;
};
