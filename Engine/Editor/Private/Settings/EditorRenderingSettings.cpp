#include "PCH.h"

#include "Settings/EditorRenderingSettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
	constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleEditor.EditorRenderingSettings";

	EditorRayTracingTopLevelMode ParseRayTracingTopLevelMode(std::string_view text) noexcept
	{
		return Strings::EqualsIgnoreCase(Strings::TrimAsciiWhitespace(text), "PartitionedTlas") ? EditorRayTracingTopLevelMode::PartitionedTlas :
		                                                                                           EditorRayTracingTopLevelMode::ClassicTlas;
	}

	EditorPtlasUpdatePath ParsePtlasUpdatePath(std::string_view text) noexcept
	{
		const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
		if (Strings::EqualsIgnoreCase(trimmed, "GpuLogicalDirtyCpuNativePack"))
		{
			return EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
		}
		if (Strings::EqualsIgnoreCase(trimmed, "FullGpuNativePack"))
		{
			return EditorPtlasUpdatePath::FullGpuNativePack;
		}
		return EditorPtlasUpdatePath::CpuPack;
	}

	const char* ToConfigString(EditorRayTracingTopLevelMode mode) noexcept
	{
		return mode == EditorRayTracingTopLevelMode::PartitionedTlas ? "PartitionedTlas" : "ClassicTlas";
	}

	const char* ToConfigString(EditorPtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return "GpuLogicalDirtyCpuNativePack";
			case EditorPtlasUpdatePath::FullGpuNativePack:
				return "FullGpuNativePack";
			case EditorPtlasUpdatePath::CpuPack:
			default:
				return "CpuPack";
		}
	}

	ERhiPartitionedTlasOperationWriterPath ToRhiPtlasPath(EditorPtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack;
			case EditorPtlasUpdatePath::FullGpuNativePack:
				return ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack;
			case EditorPtlasUpdatePath::CpuPack:
			default:
				return ERhiPartitionedTlasOperationWriterPath::CpuPack;
		}
	}

	EditorPtlasUpdatePath FromRhiPtlasPath(ERhiPartitionedTlasOperationWriterPath path) noexcept
	{
		switch (path)
		{
			case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
				return EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
			case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
				return EditorPtlasUpdatePath::FullGpuNativePack;
			case ERhiPartitionedTlasOperationWriterPath::CpuPack:
			case ERhiPartitionedTlasOperationWriterPath::None:
			default:
				return EditorPtlasUpdatePath::CpuPack;
		}
	}
}

EditorRenderingSettingsSection::EditorRenderingSettingsSection() : EditorConfigBackedSettingsSection(kRenderingSettingsSection)
{
	RefreshFromRuntimeState();
}

void EditorRenderingSettingsSection::SetVSync(bool enabled)
{
	EditorRenderingSettingsState state = GetState();
	if (state.VSync == enabled)
	{
		return;
	}
	state.VSync = enabled;
	UpdateState(state);
}

void EditorRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	EditorRenderingSettingsState state = GetState();
	if (state.PreferHighPerformanceAdapter == enabled)
	{
		return;
	}
	state.PreferHighPerformanceAdapter = enabled;
	UpdateState(state);
}

void EditorRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	EditorRenderingSettingsState state = GetState();
	if (state.MeshAutoBatching == enabled)
	{
		return;
	}
	state.MeshAutoBatching = enabled;
	UpdateState(state);
}

void EditorRenderingSettingsSection::SetRayTracingTopLevelMode(EditorRayTracingTopLevelMode mode)
{
	EditorRenderingSettingsState state = GetState();
	if (state.RayTracingTopLevelMode == mode)
	{
		return;
	}
	state.RayTracingTopLevelMode = mode;
	UpdateState(state);
}

void EditorRenderingSettingsSection::SetPtlasUpdatePath(EditorPtlasUpdatePath path)
{
	EditorRenderingSettingsState state = GetState();
	if (state.PtlasUpdatePath == path)
	{
		return;
	}
	state.PtlasUpdatePath = path;
	UpdateState(state);
}

EditorRenderingSettingsState EditorRenderingSettingsSection::CaptureRuntimeState() const noexcept
{
	EditorRenderingSettingsState state;
	state.VSync = CVarRhiVSync.Get();
	state.PreferHighPerformanceAdapter = CVarRhiPreferHighPerformanceAdapter.Get();
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
	state.RayTracingTopLevelMode =
	    CVarRhiRayTracingPreferPartitionedTlas.Get() ? EditorRayTracingTopLevelMode::PartitionedTlas : EditorRayTracingTopLevelMode::ClassicTlas;
	state.PtlasUpdatePath = FromRhiPtlasPath(CVarRayTracingPtlasOperationWriterPath.Get());
	return state;
}

void EditorRenderingSettingsSection::ApplyStateToRuntime(const EditorRenderingSettingsState& state) const noexcept
{
	CVarRhiVSync.Set(state.VSync);
	CVarRhiPreferHighPerformanceAdapter.Set(state.PreferHighPerformanceAdapter);
	CVarRendererMeshAutoBatching.Set(state.MeshAutoBatching);
	CVarRhiRayTracingPreferPartitionedTlas.Set(state.RayTracingTopLevelMode == EditorRayTracingTopLevelMode::PartitionedTlas);
	CVarRayTracingPtlasOperationWriterPath.Set(ToRhiPtlasPath(state.PtlasUpdatePath));
}

void EditorRenderingSettingsSection::ReadConfigValue(
    EditorRenderingSettingsState& state,
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

std::vector<std::pair<std::string, std::string>> EditorRenderingSettingsSection::BuildConfigValues(
    const EditorRenderingSettingsState& state) const
{
	return {
	    {"VSync", state.VSync ? "true" : "false"},
	    {"PreferHighPerformanceAdapter", state.PreferHighPerformanceAdapter ? "true" : "false"},
	    {"MeshAutoBatching", state.MeshAutoBatching ? "true" : "false"},
	    {"RayTracingTopLevelMode", ToConfigString(state.RayTracingTopLevelMode)},
	    {"PtlasUpdatePath", ToConfigString(state.PtlasUpdatePath)},
	};
}

bool EditorRenderingSettingsSection::ComputePendingRestart(
    const EditorRenderingSettingsState& baseline,
    const EditorRenderingSettingsState& current) const noexcept
{
	return baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter ||
	       baseline.RayTracingTopLevelMode != current.RayTracingTopLevelMode;
}

std::string EditorRenderingSettingsSection::DescribePendingRestart(
    const EditorRenderingSettingsState& baseline,
    const EditorRenderingSettingsState& current) const
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
	stream << "Restart the editor to apply ";
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
