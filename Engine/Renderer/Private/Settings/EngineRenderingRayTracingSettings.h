#pragma once

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <string>
#include <string_view>
#include <vector>

namespace EngineRenderingRayTracingSettings
{
	std::uint32_t SanitizePtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis) noexcept;
	std::uint32_t SanitizeIndirectBounceCount(std::uint32_t bounceCount) noexcept;
	float SanitizePtlasModeChangeDistance(float distance) noexcept;
	void Capture(EngineRenderingSettingsState& state) noexcept;
	void Apply(const EngineRenderingSettingsState& state) noexcept;
	bool ReadConfigValue(EngineRenderingSettingsState& state, std::string_view key, std::string_view value);
	void AppendConfigValues(
	    const EngineRenderingSettingsState& state,
	    std::vector<std::pair<std::string, std::string>>& values);
}
