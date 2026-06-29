#pragma once

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <string>
#include <string_view>
#include <vector>

namespace EngineRenderingDisplaySettings
{
	PixelFormat ParseBackBufferFormat(std::string_view text) noexcept;
	EngineToneMapper ParseToneMapper(std::string_view text) noexcept;
	EngineExposureMode ParseExposureMode(std::string_view text) noexcept;
	EngineExposureMeteringMethod ParseExposureMeteringMethod(std::string_view text) noexcept;
	EngineOutputColorEncoding ParseOutputColorEncoding(std::string_view text) noexcept;
	void Capture(EngineRenderingSettingsState& state) noexcept;
	void Apply(const EngineRenderingSettingsState& state) noexcept;
	bool ReadConfigValue(EngineRenderingSettingsState& state, std::string_view key, std::string_view value);
	void AppendConfigValues(
	    const EngineRenderingSettingsState& state,
	    std::vector<std::pair<std::string, std::string>>& values);
	bool RequiresRestart(const EngineRenderingSettingsState& baseline, const EngineRenderingSettingsState& current) noexcept;
	void AppendRestartReasons(
	    const EngineRenderingSettingsState& baseline,
	    const EngineRenderingSettingsState& current,
	    std::vector<std::string>& reasons);
}
