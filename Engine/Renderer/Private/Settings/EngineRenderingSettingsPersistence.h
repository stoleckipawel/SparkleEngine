#pragma once

#include <filesystem>
#include <span>
#include <string_view>

struct EngineRenderingSettingsState;

class EngineRenderingSettingsPersistence final
{
public:
	using LoadVisitor = void (*)(std::string_view key, std::string_view value);

	static void Load(LoadVisitor visitor);
	static void Write(const EngineRenderingSettingsState& state);
	static bool IsPersistedName(std::string_view name) noexcept;

private:
	static constexpr std::string_view GetSectionName() noexcept { return "/Script/SparkleRenderer.EngineRenderingSettings"; }
	static std::filesystem::path GetConfigPath();
	static std::span<const std::string_view> GetPersistedNames() noexcept;
};
