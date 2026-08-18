#pragma once

#include <string_view>

struct EngineRenderingSettingsState;

class EngineRenderingSettingsRuntime final
{
public:
	static EngineRenderingSettingsState Capture() noexcept;
	static void Apply(const EngineRenderingSettingsState& state) noexcept;
	static void ApplyPersistedValues() noexcept;

private:
	static void ApplyPersistedValue(std::string_view key, std::string_view value);
};
