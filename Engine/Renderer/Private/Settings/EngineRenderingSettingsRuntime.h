#pragma once

struct EngineRenderingSettingsState;

class EngineRenderingSettingsRuntime final
{
public:
	static EngineRenderingSettingsState Capture() noexcept;
	static void Apply(const EngineRenderingSettingsState& state) noexcept;
};
