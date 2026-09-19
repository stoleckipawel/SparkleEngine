#pragma once

#include <filesystem>
#include <span>
#include <string_view>

struct EngineRenderingSettingsState;

class EngineRenderingSettingsPersistence final
{
public:
	static void Apply() noexcept;
	static void Write(const EngineRenderingSettingsState& state);

private:
	static constexpr std::string_view GetSectionName() noexcept { return "/Script/SparkleRenderer.EngineRenderingSettings"; }
	static std::filesystem::path GetConfigPath();
	static std::span<const std::string_view> GetPersistedNames() noexcept;
	static bool IsPersistedName(std::string_view name) noexcept;
};
