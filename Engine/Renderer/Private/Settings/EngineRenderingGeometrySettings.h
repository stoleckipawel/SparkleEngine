#pragma once

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <string>
#include <string_view>
#include <vector>

namespace EngineRenderingGeometrySettings
{
	void Capture(EngineRenderingSettingsState& state) noexcept;
	void Apply(const EngineRenderingSettingsState& state) noexcept;
	bool ReadConfigValue(EngineRenderingSettingsState& state, std::string_view key, std::string_view value);
	void AppendConfigValues(
	    const EngineRenderingSettingsState& state,
	    std::vector<std::pair<std::string, std::string>>& values);
}
