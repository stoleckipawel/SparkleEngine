#include "PCH.h"

#include "Panels/RenderingSettingsPanel.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <cfloat>
#include <string>

namespace
{
	constexpr float kLabelColumnWidth = 340.0f;

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
	void DrawFloatInputRow(const char* id, const char* label, float value, OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		float updatedValue = value;
		if (ImGui::InputFloat(id, &updatedValue, 1.0f, 10.0f, "%.3f"))
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

	if (MatchesFilter(filterText, "Display", "display vsync high-performance adapter gpu") && BeginSettingsCategory("Display"))
	{
		if (BeginSettingsTable("##RenderingDisplaySettings"))
		{
			DrawBooleanRow("##VSync", "VSync", settings.VSync, [this](bool value) { m_settings->SetVSync(value); });
			DrawBooleanRow(
			    "##PreferHighPerformanceAdapter",
			    "Prefer high-performance adapter",
			    settings.PreferHighPerformanceAdapter,
			    [this](bool value) { m_settings->SetPreferHighPerformanceAdapter(value); });
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
	        "ray tracing tlas refit ptlas active partition update mode partitions topology xz xyz dynamic distance") &&
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
			ImGui::EndTable();
		}
	}

	ImGui::EndDisabled();
}
