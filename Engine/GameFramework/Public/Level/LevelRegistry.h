#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Level;

class SPARKLE_ENGINE_API LevelRegistry final
{
  public:
	LevelRegistry();
	~LevelRegistry() noexcept;

	LevelRegistry(const LevelRegistry&) = delete;
	LevelRegistry& operator=(const LevelRegistry&) = delete;
	LevelRegistry(LevelRegistry&&) = delete;
	LevelRegistry& operator=(LevelRegistry&&) = delete;

	Level* FindLevel(std::string_view name) const;

	Level* FindLevelOrDefault(std::string_view name) const;

	const std::unordered_map<std::string, std::unique_ptr<Level>>& GetAllLevels() const noexcept;

	std::size_t GetLevelCount() const noexcept;

	void SetDefaultLevelName(std::string_view name);
	bool SaveLevelCameraDefaults(std::string_view levelName, const CameraDesc& cameraDesc, std::string* errorMessage = nullptr);

	std::string_view GetDefaultLevelName() const noexcept;
	Level* GetDefaultLevel() const;

  private:
	void DiscoverLevels();
	void Register(std::unique_ptr<Level> level);

	std::unordered_map<std::string, std::unique_ptr<Level>> m_levels;
	std::string m_defaultLevelName;
};
