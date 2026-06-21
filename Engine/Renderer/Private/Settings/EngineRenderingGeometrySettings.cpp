#include "PCH.h"

#include "Settings/EngineRenderingGeometrySettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Renderer/Public/Debug/RendererCVars.h"

void EngineRenderingGeometrySettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
}

void EngineRenderingGeometrySettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	CVarRendererMeshAutoBatching.Set(state.MeshAutoBatching);
}

bool EngineRenderingGeometrySettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	if (trimmedKey != "MeshAutoBatching")
	{
		return false;
	}

	(void)Strings::TryParseBool(Strings::TrimCopy(value), state.MeshAutoBatching);
	return true;
}

void EngineRenderingGeometrySettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("MeshAutoBatching", state.MeshAutoBatching ? "true" : "false");
}
