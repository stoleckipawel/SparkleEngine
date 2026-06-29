#include "PCH.h"

#include "Panels/RenderingSettingsPanel.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <string>

namespace
{
	constexpr float kLabelColumnWidth = 340.0f;

	int ToBackBufferFormatIndex(PixelFormat format) noexcept
	{
		switch (format)
		{
			case PixelFormat::R8G8B8A8_UNorm_Srgb:
				return 1;
			case PixelFormat::B8G8R8A8_UNorm:
				return 2;
			case PixelFormat::B8G8R8A8_UNorm_Srgb:
				return 3;
			case PixelFormat::R8G8B8A8_UNorm:
			default:
				return 0;
		}
	}

	PixelFormat FromBackBufferFormatIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return PixelFormat::R8G8B8A8_UNorm_Srgb;
			case 2:
				return PixelFormat::B8G8R8A8_UNorm;
			case 3:
				return PixelFormat::B8G8R8A8_UNorm_Srgb;
			case 0:
			default:
				return PixelFormat::R8G8B8A8_UNorm;
		}
	}

	int ToToneMapperIndex(EngineToneMapper toneMapper) noexcept
	{
		switch (toneMapper)
		{
			case EngineToneMapper::Reinhard:
				return 0;
			case EngineToneMapper::AcesFilmic:
				return 2;
			case EngineToneMapper::AcesApprox:
			default:
				return 1;
		}
	}

	EngineToneMapper FromToneMapperIndex(int index) noexcept
	{
		switch (index)
		{
			case 0:
				return EngineToneMapper::Reinhard;
			case 2:
				return EngineToneMapper::AcesFilmic;
			case 1:
			default:
				return EngineToneMapper::AcesApprox;
		}
	}

	int ToExposureModeIndex(EngineExposureMode mode) noexcept
	{
		return mode == EngineExposureMode::Manual ? 0 : 1;
	}

	EngineExposureMode FromExposureModeIndex(int index) noexcept
	{
		return index == 0 ? EngineExposureMode::Manual : EngineExposureMode::Automatic;
	}

	int ToExposureMeteringMethodIndex(EngineExposureMeteringMethod method) noexcept
	{
		switch (method)
		{
			case EngineExposureMeteringMethod::DownsamplePyramid:
				return 1;
			case EngineExposureMeteringMethod::ParallelReduction:
			default:
				return 0;
		}
	}

	EngineExposureMeteringMethod FromExposureMeteringMethodIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EngineExposureMeteringMethod::DownsamplePyramid;
			case 0:
			default:
				return EngineExposureMeteringMethod::ParallelReduction;
		}
	}

	int ToOutputColorEncodingIndex(EngineOutputColorEncoding encoding) noexcept
	{
		switch (encoding)
		{
			case EngineOutputColorEncoding::Linear:
				return 1;
			case EngineOutputColorEncoding::Srgb:
				return 2;
			case EngineOutputColorEncoding::Automatic:
			default:
				return 0;
		}
	}

	EngineOutputColorEncoding FromOutputColorEncodingIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EngineOutputColorEncoding::Linear;
			case 2:
				return EngineOutputColorEncoding::Srgb;
			case 0:
			default:
				return EngineOutputColorEncoding::Automatic;
		}
	}

	int ToPtlasPartitionUpdateModeIndex(EnginePtlasPartitionUpdateMode mode) noexcept
	{
		switch (mode)
		{
			case EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal:
				return 1;
			case EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise:
				return 2;
			case EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition:
			default:
				return 0;
		}
	}

	EnginePtlasPartitionUpdateMode FromPtlasPartitionUpdateModeIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal;
			case 2:
				return EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
			case 0:
			default:
				return EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition;
		}
	}

	int ToPtlasPartitionTopologyIndex(EnginePtlasPartitionTopology topology) noexcept
	{
		return topology == EnginePtlasPartitionTopology::XYZ3D ? 1 : 0;
	}

	EnginePtlasPartitionTopology FromPtlasPartitionTopologyIndex(int index) noexcept
	{
		return index == 1 ? EnginePtlasPartitionTopology::XYZ3D : EnginePtlasPartitionTopology::XZ2D;
	}

	bool MatchesFilter(const char* filterText, const char* title, const char* keywords)
	{
		if (filterText == nullptr || filterText[0] == '\0')
		{
			return true;
		}

		return UiUtil::MatchesDetailsFilter(std::string(filterText), title, keywords);
	}

	bool BeginSettingsCategory(const char* label)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, SparkleUiPalette::HeaderBackground());
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SparkleUiPalette::HeaderBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, SparkleUiPalette::HeaderBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
		const bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);
		return open;
	}

	bool BeginSettingsTable(const char* id)
	{
		const ImGuiTableFlags tableFlags =
		    ImGuiTableFlags_SizingStretchProp |
		    ImGuiTableFlags_BordersInnerV |
		    ImGuiTableFlags_BordersInnerH;
		if (!ImGui::BeginTable(id, 2, tableFlags))
		{
			return false;
		}

		ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	template <typename OnChanged>
	void DrawBooleanRow(const char* id, const char* label, bool value, OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		bool updatedValue = value;
		if (ImGui::Checkbox(id, &updatedValue))
		{
			onChanged(updatedValue);
		}
	}

	template <typename OnChanged>
	void DrawComboRow(const char* id, const char* label, int value, const char* const labels[], int labelCount, OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		int updatedValue = value;
		if (ImGui::Combo(id, &updatedValue, labels, labelCount))
		{
			onChanged(updatedValue);
		}
	}

	template <typename OnChanged>
	void DrawUnsignedIntInputRow(
	    const char* id,
	    const char* label,
	    std::uint32_t value,
	    OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		int updatedValue = static_cast<int>(value);
		if (ImGui::InputInt(id, &updatedValue, 1, 16))
		{
			onChanged(static_cast<std::uint32_t>((std::max)(updatedValue, 0)));
		}
	}

	template <typename OnChanged>
	void DrawUnsignedIntSliderRow(
	    const char* id,
	    const char* label,
	    std::uint32_t value,
	    std::uint32_t minValue,
	    std::uint32_t maxValue,
	    OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		int updatedValue = static_cast<int>(value);
		const int minInt = static_cast<int>(minValue);
		const int maxInt = static_cast<int>(maxValue);
		if (ImGui::SliderInt(id, &updatedValue, minInt, maxInt))
		{
			onChanged(static_cast<std::uint32_t>(updatedValue));
		}
	}

	template <typename OnChanged>
	void DrawFloatInputRow(
	    const char* id,
	    const char* label,
	    float value,
	    OnChanged&& onChanged,
	    float step = 1.0f,
	    float stepFast = 10.0f,
	    const char* format = "%.3f")
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		float updatedValue = value;
		if (ImGui::InputFloat(id, &updatedValue, step, stepFast, format))
		{
			onChanged(updatedValue);
		}
	}
}

void RenderingSettingsPanel::SetSettings(EngineRenderingSettingsSection* settings) noexcept
{
	m_settings = settings;
}

void RenderingSettingsPanel::RefreshFromRuntimeState() noexcept
{
	if (m_settings != nullptr)
	{
		m_settings->RefreshFromRuntimeState();
	}
}

bool RenderingSettingsPanel::HasPendingRestart() const noexcept
{
	return m_settings != nullptr && m_settings->HasPendingRestart();
}

void RenderingSettingsPanel::BuildUI(bool disableInteraction, const char* filterText)
{
	if (m_settings == nullptr)
	{
		return;
	}

	const EngineRenderingSettingsState& settings = m_settings->GetState();

	ImGui::TextUnformatted("Engine - Rendering");
	ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
	ImGui::TextUnformatted("Rendering settings.");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, 2.0f));

	if (HasPendingRestart())
	{
		const std::string restartMessage = m_settings->BuildPendingRestartMessage();
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::AccentStrong());
		ImGui::TextWrapped("%s", restartMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	ImGui::BeginDisabled(disableInteraction);

	static constexpr const char* backBufferFormatLabels[] = {
	    "R8G8B8A8 UNorm",
	    "R8G8B8A8 sRGB",
	    "B8G8R8A8 UNorm",
	    "B8G8R8A8 sRGB",
	};
	static constexpr const char* toneMapperLabels[] = {
	    "Reinhard",
	    "ACES approximate",
	    "ACES fitted filmic",
	};
	static constexpr const char* exposureModeLabels[] = {
	    "Manual",
	    "Automatic",
	};
	static constexpr const char* exposureMeteringMethodLabels[] = {
	    "Parallel reduction",
	    "Downsample pyramid",
	};
	static constexpr const char* outputColorEncodingLabels[] = {
	    "Automatic",
	    "Linear",
	    "sRGB",
	};
	if (MatchesFilter(
	        filterText,
	        "Display",
	        "display vsync high-performance adapter gpu back buffer format tone mapper aces reinhard exposure automatic manual metering reduction downsample pyramid compensation luminance sdr srgb output encoding") &&
	    BeginSettingsCategory("Display"))
	{
		if (BeginSettingsTable("##RenderingDisplaySettings"))
		{
			DrawBooleanRow("##VSync", "VSync", settings.VSync, [this](bool value) { m_settings->SetVSync(value); });
			DrawComboRow(
			    "##BackBufferFormat",
			    "Back buffer format",
			    ToBackBufferFormatIndex(settings.BackBufferFormat),
			    backBufferFormatLabels,
			    IM_ARRAYSIZE(backBufferFormatLabels),
			    [this](int value) { m_settings->SetBackBufferFormat(FromBackBufferFormatIndex(value)); });
			DrawBooleanRow(
			    "##PreferHighPerformanceAdapter",
			    "Prefer high-performance adapter",
			    settings.PreferHighPerformanceAdapter,
			    [this](bool value) { m_settings->SetPreferHighPerformanceAdapter(value); });
			DrawComboRow(
			    "##ToneMapper",
			    "Tone mapper",
			    ToToneMapperIndex(settings.ToneMapper),
			    toneMapperLabels,
			    IM_ARRAYSIZE(toneMapperLabels),
			    [this](int value) { m_settings->SetToneMapper(FromToneMapperIndex(value)); });
			DrawComboRow(
			    "##ExposureMode",
			    "Exposure mode",
			    ToExposureModeIndex(settings.ExposureMode),
			    exposureModeLabels,
			    IM_ARRAYSIZE(exposureModeLabels),
			    [this](int value) { m_settings->SetExposureMode(FromExposureModeIndex(value)); });
			DrawComboRow(
			    "##ExposureMeteringMethod",
			    "Exposure metering",
			    ToExposureMeteringMethodIndex(settings.ExposureMeteringMethod),
			    exposureMeteringMethodLabels,
			    IM_ARRAYSIZE(exposureMeteringMethodLabels),
			    [this](int value) { m_settings->SetExposureMeteringMethod(FromExposureMeteringMethodIndex(value)); });
			DrawComboRow(
			    "##OutputColorEncoding",
			    "Output encoding",
			    ToOutputColorEncodingIndex(settings.OutputColorEncoding),
			    outputColorEncodingLabels,
			    IM_ARRAYSIZE(outputColorEncodingLabels),
			    [this](int value) { m_settings->SetOutputColorEncoding(FromOutputColorEncodingIndex(value)); });
			ImGui::BeginDisabled(settings.ExposureMode != EngineExposureMode::Manual);
			DrawFloatInputRow(
			    "##ManualExposure",
			    "Manual exposure",
			    settings.ManualExposure,
			    [this](float value) { m_settings->SetManualExposure(value); },
			    0.1f,
			    1.0f,
			    "%.4f");
			ImGui::EndDisabled();
			DrawFloatInputRow(
			    "##ExposureCompensation",
			    "Exposure compensation EV",
			    settings.ExposureCompensation,
			    [this](float value) { m_settings->SetExposureCompensation(value); },
			    0.1f,
			    1.0f,
			    "%.2f");
			DrawFloatInputRow(
			    "##ExposureTargetLuminance",
			    "Target luminance",
			    settings.ExposureTargetLuminance,
			    [this](float value) { m_settings->SetExposureTargetLuminance(value); },
			    0.01f,
			    0.1f,
			    "%.4f");
			DrawFloatInputRow(
			    "##ExposureMin",
			    "Min exposure",
			    settings.ExposureMin,
			    [this](float value) { m_settings->SetExposureMin(value); },
			    0.0001f,
			    0.01f,
			    "%.6f");
			DrawFloatInputRow(
			    "##ExposureMax",
			    "Max exposure",
			    settings.ExposureMax,
			    [this](float value) { m_settings->SetExposureMax(value); },
			    1.0f,
			    64.0f,
			    "%.3f");
			DrawFloatInputRow(
			    "##ExposureAdaptationSpeedUp",
			    "Adapt speed up",
			    settings.ExposureAdaptationSpeedUp,
			    [this](float value) { m_settings->SetExposureAdaptationSpeedUp(value); },
			    0.1f,
			    1.0f,
			    "%.3f");
			DrawFloatInputRow(
			    "##ExposureAdaptationSpeedDown",
			    "Adapt speed down",
			    settings.ExposureAdaptationSpeedDown,
			    [this](float value) { m_settings->SetExposureAdaptationSpeedDown(value); },
			    0.1f,
			    1.0f,
			    "%.3f");
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	if (MatchesFilter(filterText, "Geometry", "geometry mesh auto batching") && BeginSettingsCategory("Geometry"))
	{
		if (BeginSettingsTable("##RenderingGeometrySettings"))
		{
			DrawBooleanRow(
			    "##MeshAutoBatching",
			    "Mesh auto batching",
			    settings.MeshAutoBatching,
			    [this](bool value) { m_settings->SetMeshAutoBatching(value); });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	if (MatchesFilter(filterText, "Lighting", "lighting light budget directional point spot") && BeginSettingsCategory("Lighting"))
	{
		if (BeginSettingsTable("##RenderingLightingSettings"))
		{
			DrawUnsignedIntInputRow(
			    "##MaxDirectionalLights",
			    "Max directional lights",
			    settings.MaxDirectionalLights,
			    [this](std::uint32_t value) { m_settings->SetMaxDirectionalLights(value); });
			DrawUnsignedIntInputRow(
			    "##MaxPointLights",
			    "Max point lights",
			    settings.MaxPointLights,
			    [this](std::uint32_t value) { m_settings->SetMaxPointLights(value); });
			DrawUnsignedIntInputRow(
			    "##MaxSpotLights",
			    "Max spot lights",
			    settings.MaxSpotLights,
			    [this](std::uint32_t value) { m_settings->SetMaxSpotLights(value); });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	static constexpr const char* ptlasPartitionTopologyLabels[] = {
	    "2D X/Z",
	    "3D X/Y/Z",
	};
	static constexpr const char* ptlasPartitionUpdateModeLabels[] = {
	    "Always update partition",
	    "Always move dynamic to global",
	    "Update partition nearby, move to global otherwise",
	};
	if (MatchesFilter(
	        filterText,
	        "Ray Tracing",
	        "ray tracing tlas refit ptlas active partition update mode partitions topology xz xyz dynamic distance indirect diffuse specular reflection bounces") &&
	    BeginSettingsCategory("Ray Tracing"))
	{
		if (BeginSettingsTable("##RenderingRayTracingSettings"))
		{
			DrawBooleanRow(
			    "##PtlasActive",
			    "PTLAS Active",
			    settings.PtlasActive,
			    [this](bool value) { m_settings->SetPtlasActive(value); });
			ImGui::BeginDisabled(settings.PtlasActive);
			DrawBooleanRow(
			    "##RefitTlas",
			    "Refit TLAS",
			    settings.RefitTlas,
			    [this](bool value) { m_settings->SetRefitTlas(value); });
			ImGui::EndDisabled();
			DrawUnsignedIntSliderRow(
			    "##PtlasPartitionsPerAxis",
			    "PTLAS partitions per axis",
			    settings.PtlasPartitionsPerAxis,
			    1u,
			    64u,
			    [this](std::uint32_t value) { m_settings->SetPtlasPartitionsPerAxis(value); });
			DrawComboRow(
			    "##PtlasPartitionTopology",
			    "PTLAS partition topology",
			    ToPtlasPartitionTopologyIndex(settings.PtlasPartitionTopology),
			    ptlasPartitionTopologyLabels,
			    IM_ARRAYSIZE(ptlasPartitionTopologyLabels),
			    [this](int value) { m_settings->SetPtlasPartitionTopology(FromPtlasPartitionTopologyIndex(value)); });
			DrawComboRow(
			    "##PtlasPartitionUpdateMode",
			    "Partition update mode",
			    ToPtlasPartitionUpdateModeIndex(settings.PtlasPartitionUpdateMode),
			    ptlasPartitionUpdateModeLabels,
			    IM_ARRAYSIZE(ptlasPartitionUpdateModeLabels),
			    [this](int value) { m_settings->SetPtlasPartitionUpdateMode(FromPtlasPartitionUpdateModeIndex(value)); });
			DrawBooleanRow(
			    "##PtlasMarkAllDynamicInPartition",
			    "Mark all dynamic in partition",
			    settings.PtlasMarkAllDynamicInPartition,
			    [this](bool value) { m_settings->SetPtlasMarkAllDynamicInPartition(value); });
			DrawFloatInputRow(
			    "##PtlasModeChangeDistance",
			    "Mode change distance",
			    settings.PtlasModeChangeDistance,
			    [this](float value) { m_settings->SetPtlasModeChangeDistance(value); });
			DrawUnsignedIntSliderRow(
			    "##IndirectDiffuseBounceCount",
			    "Indirect diffuse bounces",
			    settings.IndirectDiffuseBounceCount,
			    1u,
			    8u,
			    [this](std::uint32_t value) { m_settings->SetIndirectDiffuseBounceCount(value); });
			DrawUnsignedIntSliderRow(
			    "##IndirectSpecularBounceCount",
			    "Indirect specular bounces",
			    settings.IndirectSpecularBounceCount,
			    1u,
			    8u,
			    [this](std::uint32_t value) { m_settings->SetIndirectSpecularBounceCount(value); });
			ImGui::EndTable();
		}
	}

	ImGui::EndDisabled();
}
