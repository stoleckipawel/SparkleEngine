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

	int ToTlasModeIndex(EngineRayTracingTopLevelMode mode) noexcept
	{
		return mode == EngineRayTracingTopLevelMode::PartitionedTlas ? 1 : 0;
	}

	EngineRayTracingTopLevelMode FromTlasModeIndex(int index) noexcept
	{
		return index == 1 ? EngineRayTracingTopLevelMode::PartitionedTlas : EngineRayTracingTopLevelMode::ClassicTlas;
	}

	int ToPtlasUpdatePathIndex(EnginePtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return 1;
			case EnginePtlasUpdatePath::FullGpuNativePack:
				return 2;
			case EnginePtlasUpdatePath::CpuPack:
			default:
				return 0;
		}
	}

	EnginePtlasUpdatePath FromPtlasUpdatePathIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EnginePtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
			case 2:
				return EnginePtlasUpdatePath::FullGpuNativePack;
			case 0:
			default:
				return EnginePtlasUpdatePath::CpuPack;
		}
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
	ImGui::Dummy(ImVec2(0.0f, 6.0f));

	if (HasPendingRestart())
	{
		const std::string restartMessage = m_settings->BuildPendingRestartMessage();
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::AccentStrong());
		ImGui::TextWrapped("%s", restartMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
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
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
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
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	}

	static constexpr const char* tlasModeLabels[] = {"Classic TLAS", "Partitioned TLAS"};
	static constexpr const char* ptlasPathLabels[] = {
	    "CPU pack",
	    "GPU dirty + CPU native pack",
	    "Full GPU native pack",
	};
	if (MatchesFilter(filterText, "Ray Tracing", "ray tracing tlas ptlas update path") && BeginSettingsCategory("Ray Tracing"))
	{
		if (BeginSettingsTable("##RenderingRayTracingSettings"))
		{
			DrawComboRow(
			    "##RayTracingTlas",
			    "Ray tracing TLAS",
			    ToTlasModeIndex(settings.RayTracingTopLevelMode),
			    tlasModeLabels,
			    IM_ARRAYSIZE(tlasModeLabels),
			    [this](int value) { m_settings->SetRayTracingTopLevelMode(FromTlasModeIndex(value)); });
			DrawComboRow(
			    "##PtlasUpdatePath",
			    "PTLAS update path",
			    ToPtlasUpdatePathIndex(settings.PtlasUpdatePath),
			    ptlasPathLabels,
			    IM_ARRAYSIZE(ptlasPathLabels),
			    [this](int value) { m_settings->SetPtlasUpdatePath(FromPtlasUpdatePathIndex(value)); });
			ImGui::EndTable();
		}
	}

	ImGui::EndDisabled();
}
