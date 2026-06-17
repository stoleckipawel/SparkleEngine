#include "PCH.h"

#include "Panels/SettingsPanel.h"

#include "Settings/EditorRenderingSettings.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <string>

namespace
{
	int ToTlasModeIndex(EditorRayTracingTopLevelMode mode) noexcept
	{
		return mode == EditorRayTracingTopLevelMode::PartitionedTlas ? 1 : 0;
	}

	EditorRayTracingTopLevelMode FromTlasModeIndex(int index) noexcept
	{
		return index == 1 ? EditorRayTracingTopLevelMode::PartitionedTlas : EditorRayTracingTopLevelMode::ClassicTlas;
	}

	int ToPtlasUpdatePathIndex(EditorPtlasUpdatePath path) noexcept
	{
		switch (path)
		{
			case EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack:
				return 1;
			case EditorPtlasUpdatePath::FullGpuNativePack:
				return 2;
			case EditorPtlasUpdatePath::CpuPack:
			default:
				return 0;
		}
	}

	EditorPtlasUpdatePath FromPtlasUpdatePathIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EditorPtlasUpdatePath::GpuLogicalDirtyCpuNativePack;
			case 2:
				return EditorPtlasUpdatePath::FullGpuNativePack;
			case 0:
			default:
				return EditorPtlasUpdatePath::CpuPack;
		}
	}
}

void SettingsPanel::SetOpen(bool open) noexcept
{
	if (open && !m_isOpen)
	{
		m_refreshFromRuntimeOnNextOpen = true;
	}
	m_isOpen = open;
}

void SettingsPanel::SetRenderingSettings(EditorRenderingSettingsSection* renderingSettings) noexcept
{
	m_renderingSettings = renderingSettings;
}

void SettingsPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen || m_renderingSettings == nullptr)
	{
		return;
	}

	if (m_refreshFromRuntimeOnNextOpen)
	{
		m_renderingSettings->RefreshFromRuntimeState();
		m_refreshFromRuntimeOnNextOpen = false;
	}

	ImGui::SetNextWindowSize(ImVec2(1040.0f, 760.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Settings, "Settings") + "##EditorSettings";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawNavigation();
	ImGui::SameLine();

	ImGui::BeginChild("##SettingsContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	switch (m_activeSection)
	{
		case Section::Rendering:
		default:
			DrawRenderingSection(disableInteraction);
			break;
	}
	ImGui::EndChild();

	ImGui::End();
}

void SettingsPanel::DrawNavigation()
{
	ImGui::BeginChild("##SettingsNavigation", ImVec2(210.0f, 0.0f), ImGuiChildFlags_Borders);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));

	const bool renderingSelected = m_activeSection == Section::Rendering;
	if (ImGui::Selectable(UiUtil::MakeIconLabel(UiUtil::EditorIcon::Material, "Rendering").c_str(), renderingSelected, 0, ImVec2(-1.0f, 0.0f)))
	{
		m_activeSection = Section::Rendering;
	}

	ImGui::PopStyleVar(2);
	ImGui::EndChild();
}

void SettingsPanel::DrawRenderingSection(bool disableInteraction)
{
	const EditorRenderingSettingsState& settings = m_renderingSettings->GetState();

	ImGui::TextUnformatted("Engine - Rendering");
	ImGui::Spacing();

	if (m_renderingSettings->HasPendingRestart())
	{
		const std::string restartMessage = m_renderingSettings->BuildPendingRestartMessage();
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::AccentStrong());
		ImGui::TextWrapped("%s", restartMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Spacing();
	}

	ImGui::BeginDisabled(disableInteraction);

	bool vsync = settings.VSync;
	if (ImGui::Checkbox("VSync", &vsync))
	{
		m_renderingSettings->SetVSync(vsync);
	}

	bool highPerformanceAdapter = settings.PreferHighPerformanceAdapter;
	if (ImGui::Checkbox("Prefer high-performance adapter", &highPerformanceAdapter))
	{
		m_renderingSettings->SetPreferHighPerformanceAdapter(highPerformanceAdapter);
	}

	bool meshAutoBatching = settings.MeshAutoBatching;
	if (ImGui::Checkbox("Mesh auto batching", &meshAutoBatching))
	{
		m_renderingSettings->SetMeshAutoBatching(meshAutoBatching);
	}

	static constexpr const char* tlasModeLabels[] = {"Classic TLAS", "Partitioned TLAS"};
	int tlasModeIndex = ToTlasModeIndex(settings.RayTracingTopLevelMode);
	if (ImGui::Combo("Ray tracing TLAS", &tlasModeIndex, tlasModeLabels, IM_ARRAYSIZE(tlasModeLabels)))
	{
		m_renderingSettings->SetRayTracingTopLevelMode(FromTlasModeIndex(tlasModeIndex));
	}

	static constexpr const char* ptlasPathLabels[] = {
	    "CPU pack",
	    "GPU dirty + CPU native pack",
	    "Full GPU native pack",
	};
	int ptlasPathIndex = ToPtlasUpdatePathIndex(settings.PtlasUpdatePath);
	if (ImGui::Combo("PTLAS update path", &ptlasPathIndex, ptlasPathLabels, IM_ARRAYSIZE(ptlasPathLabels)))
	{
		m_renderingSettings->SetPtlasUpdatePath(FromPtlasUpdatePathIndex(ptlasPathIndex));
	}

	ImGui::EndDisabled();
}
