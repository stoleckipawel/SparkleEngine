#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "Level/LevelRegistry.h"
#include "Runtime/Level/LevelChangeEvents.h"

#include <string>
#include <string_view>
#include <vector>

class Level;
class Scene;
class CameraController;
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
	bool HasActiveLevel() const noexcept { return m_bHasActiveLevel; }
	bool IsLevelChangeInProgress() const noexcept { return m_bLevelChangeInProgress; }
	std::vector<std::string> GetRegisteredLevelNames() const;
	LevelChangeEvents& GetLevelChangeEvents() noexcept { return m_levelChangeEvents; }
	const LevelChangeEvents& GetLevelChangeEvents() const noexcept { return m_levelChangeEvents; }

	void RequestLevelChange(std::string_view requestedLevelName) noexcept;
	void RegisterCameraController(CameraController& cameraController) noexcept;
	CameraController* GetCameraController() noexcept { return m_cameraController; }
	const CameraController* GetCameraController() const noexcept { return m_cameraController; }
	bool ResetActiveLevelCamera() noexcept;
	bool SaveActiveLevelCameraDefaults(const CameraDesc& cameraDesc) noexcept;

  private:
	static constexpr std::string_view GetEmptyLevelName() noexcept { return "Empty"; }
	static constexpr std::string_view GetStartupLevelName() noexcept { return "Sponza"; }

	void ApplyLevelCamera() noexcept;
	void InitializeStartupLevel() noexcept;
	SceneLoadResult LoadLevelFromUnloadedState(const Level& level) noexcept;
	void ProcessLevelChangeRequest(const Level& requestedLevel) noexcept;

	Scene* m_scene = nullptr;
	CameraController* m_cameraController = nullptr;
	LevelRegistry m_levelRegistry;
	LevelChangeEvents m_levelChangeEvents;
	CameraDesc m_levelCameraDesc;

	std::string m_activeLevelName;
	bool m_bHasActiveLevel = false;
	bool m_bLevelChangeInProgress = false;
};