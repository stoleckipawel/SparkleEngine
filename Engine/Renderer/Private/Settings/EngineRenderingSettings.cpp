#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
	constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleRenderer.EngineRenderingSettings";

	EngineRayTracingTopLevelMode ParseRayTracingTopLevelMode(std::string_view text) noexcept
	{
		return Strings::EqualsIgnoreCase(Strings::TrimAsciiWhitespace(text), "PartitionedTlas") ? EngineRayTracingTopLevelMode::PartitionedTlas :
		                                                                                           EngineRayTracingTopLevelMode::ClassicTlas;
	}

	EnginePtlasUpdatePath ParsePtlasUpdatePath(std::string_view text) noexcept
	{
		const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
		if (Strings::EqualsIgnoreCase(trimmed, "GpuLogicalDirtyCpuNativePack"))
		{
			return EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
		}
		if (Strings::EqualsIgnoreCase(trimmed, "FullGpuNativePack"))
		{
			return EnginePtlasUpdatePath::FullGpuNativePack;
		}
		return EnginePtlasUpdatePath::CpuPack;
	}

	const char* ToConfigString(EngineRayTracingTopLevelMode mode) noexcept
	{
		return mode == EngineRayTracingTopLevelMode::PartitionedTlas ? "PartitionedTlas" : "ClassicTlas";
	}

	const char* ToConfigString(EnginePtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return "GpuLogicalDirtyCpuNativePack";
			case EnginePtlasUpdatePath::FullGpuNativePack:
				return "FullGpuNativePack";
			case EnginePtlasUpdatePath::CpuPack:
			default:
				return "CpuPack";
		}
	}

	ERhiPartitionedTlasOperationWriterPath ToRhiPtlasPath(EnginePtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack;
			case EnginePtlasUpdatePath::FullGpuNativePack:
				return ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack;
			case EnginePtlasUpdatePath::CpuPack:
			default:
				return ERhiPartitionedTlasOperationWriterPath::CpuPack;
		}
	}

	EnginePtlasUpdatePath FromRhiPtlasPath(ERhiPartitionedTlasOperationWriterPath path) noexcept
	{
		switch (path)
		{
			case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
				return EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
			case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
				return EnginePtlasUpdatePath::FullGpuNativePack;
			case ERhiPartitionedTlasOperationWriterPath::CpuPack:
			case ERhiPartitionedTlasOperationWriterPath::None:
			default:
				return EnginePtlasUpdatePath::CpuPack;
		}
	}
}

EngineRenderingSettingsSection::EngineRenderingSettingsSection() :
	ConfigBackedSettingsSection(ConfigBackedSettings::DefaultProjectConfigPath("Engine"), kRenderingSettingsSection)
{
	RefreshFromRuntimeState();
}

void EngineRenderingSettingsSection::SetVSync(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.VSync == enabled)
	{
		return;
	}
	state.VSync = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PreferHighPerformanceAdapter == enabled)
	{
		return;
	}
	state.PreferHighPerformanceAdapter = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.MeshAutoBatching == enabled)
	{
		return;
	}
	state.MeshAutoBatching = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetRayTracingTopLevelMode(EngineRayTracingTopLevelMode mode)
{
	EngineRenderingSettingsState state = GetState();
	if (state.RayTracingTopLevelMode == mode)
	{
		return;
	}
	state.RayTracingTopLevelMode = mode;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasUpdatePath(EnginePtlasUpdatePath path)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasUpdatePath == path)
	{
		return;
	}
	state.PtlasUpdatePath = path;
	UpdateState(state);
}

EngineRenderingSettingsState EngineRenderingSettingsSection::CaptureRuntimeState() const noexcept
{
	EngineRenderingSettingsState state;
	state.VSync = CVarRhiVSync.Get();
	state.PreferHighPerformanceAdapter = CVarRhiPreferHighPerformanceAdapter.Get();
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
	state.RayTracingTopLevelMode =
	    CVarRhiRayTracingPreferPartitionedTlas.Get() ? EngineRayTracingTopLevelMode::PartitionedTlas : EngineRayTracingTopLevelMode::ClassicTlas;
	state.PtlasUpdatePath = FromRhiPtlasPath(CVarRayTracingPtlasOperationWriterPath.Get());
	return state;
}

void EngineRenderingSettingsSection::ApplyStateToRuntime(const EngineRenderingSettingsState& state) const noexcept
{
	CVarRhiVSync.Set(state.VSync);
	CVarRhiPreferHighPerformanceAdapter.Set(state.PreferHighPerformanceAdapter);
	CVarRendererMeshAutoBatching.Set(state.MeshAutoBatching);
	CVarRhiRayTracingPreferPartitionedTlas.Set(state.RayTracingTopLevelMode == EngineRayTracingTopLevelMode::PartitionedTlas);
	CVarRayTracingPtlasOperationWriterPath.Set(ToRhiPtlasPath(state.PtlasUpdatePath));
}

void EngineRenderingSettingsSection::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value) const
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	const std::string trimmedValue = Strings::TrimCopy(value);
	if (trimmedKey == "VSync")
	{
		(void) Strings::TryParseBool(trimmedValue, state.VSync);
	}
	else if (trimmedKey == "PreferHighPerformanceAdapter")
	{
		(void) Strings::TryParseBool(trimmedValue, state.PreferHighPerformanceAdapter);
	}
	else if (trimmedKey == "MeshAutoBatching")
	{
		(void) Strings::TryParseBool(trimmedValue, state.MeshAutoBatching);
	}
	else if (trimmedKey == "RayTracingTopLevelMode")
	{
		state.RayTracingTopLevelMode = ParseRayTracingTopLevelMode(trimmedValue);
	}
	else if (trimmedKey == "PtlasUpdatePath")
	{
		state.PtlasUpdatePath = ParsePtlasUpdatePath(trimmedValue);
	}
}

std::vector<std::pair<std::string, std::string>> EngineRenderingSettingsSection::BuildConfigValues(
    const EngineRenderingSettingsState& state) const
{
	return {
	    {"VSync", state.VSync ? "true" : "false"},
	    {"PreferHighPerformanceAdapter", state.PreferHighPerformanceAdapter ? "true" : "false"},
	    {"MeshAutoBatching", state.MeshAutoBatching ? "true" : "false"},
	    {"RayTracingTopLevelMode", ToConfigString(state.RayTracingTopLevelMode)},
	    {"PtlasUpdatePath", ToConfigString(state.PtlasUpdatePath)},
	};
}

bool EngineRenderingSettingsSection::ComputePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const noexcept
{
	return baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter ||
	       baseline.RayTracingTopLevelMode != current.RayTracingTopLevelMode;
}

std::string EngineRenderingSettingsSection::DescribePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const
{
	std::vector<std::string> reasons;
	if (baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter)
	{
		reasons.emplace_back("GPU adapter preference");
	}
	if (baseline.RayTracingTopLevelMode != current.RayTracingTopLevelMode)
	{
		reasons.emplace_back("ray tracing TLAS mode");
	}
	if (reasons.empty())
	{
		return {};
	}

	std::ostringstream stream;
	stream << "Restart the application to apply ";
	for (std::size_t index = 0; index < reasons.size(); ++index)
	{
		if (index > 0)
		{
			stream << (index + 1 == reasons.size() ? " and " : ", ");
		}
		stream << reasons[index];
	}
	stream << ".";
	return stream.str();
}

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
