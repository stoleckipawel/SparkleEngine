#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

class SPARKLE_ENGINE_API LevelAsset final
{
  public:
	LevelAsset(LevelDesc levelDesc, std::filesystem::path sourcePath = {});
	~LevelAsset() = default;

	LevelAsset(const LevelAsset&) = delete;
	LevelAsset& operator=(const LevelAsset&) = delete;
	LevelAsset(LevelAsset&&) = delete;
	LevelAsset& operator=(LevelAsset&&) = delete;

	std::string_view GetName() const noexcept { return m_levelDesc.name; }

	LevelDesc BuildDescription() const { return m_levelDesc; }
	const LevelDesc& GetLevelDesc() const noexcept { return m_levelDesc; }
	void SetLevelDesc(const LevelDesc& levelDesc) noexcept { m_levelDesc = levelDesc; }

	const std::filesystem::path& GetSourcePath() const noexcept { return m_sourcePath; }
	void SetSourcePath(std::filesystem::path sourcePath) noexcept { m_sourcePath = std::move(sourcePath); }

  private:
	LevelDesc m_levelDesc;
	std::filesystem::path m_sourcePath;
};
