#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

class GameCameraController;
class GameSceneLightingState;

class SPARKLE_ENGINE_API Level final
{
  public:
	Level(LevelDesc levelDesc, std::filesystem::path sourcePath = {});
	~Level() = default;

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;
	Level(Level&&) = delete;
	Level& operator=(Level&&) = delete;

	std::string_view GetName() const noexcept { return m_levelDesc.name; }

	LevelDesc BuildDescription() const { return m_levelDesc; }
	const LevelDesc& GetLevelDesc() const noexcept { return m_levelDesc; }
	const CameraDesc& GetInitialCamera() const noexcept { return m_levelDesc.cameraDesc; }
	const LevelLightingDesc& GetInitialLighting() const noexcept { return m_levelDesc.lightingDesc; }
	std::uint32_t GetDirectionalLightCount() const noexcept { return m_levelDesc.lightingDesc.directionalLightCount; }
	void SetLevelDesc(const LevelDesc& levelDesc) noexcept { m_levelDesc = levelDesc; }
	void ApplyToRuntime(GameCameraController* gameCameraController, GameSceneLightingState* lightingState) const noexcept;
	void CaptureFromRuntime(const GameCameraController* gameCameraController, const GameSceneLightingState* lightingState) noexcept;

	const std::filesystem::path& GetSourcePath() const noexcept { return m_sourcePath; }
	void SetSourcePath(std::filesystem::path sourcePath) noexcept { m_sourcePath = std::move(sourcePath); }

  private:
	LevelDesc m_levelDesc;
	std::filesystem::path m_sourcePath;
};
