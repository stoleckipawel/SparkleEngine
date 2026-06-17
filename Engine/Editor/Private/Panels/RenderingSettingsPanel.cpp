#include "PCH.h"

#include "Panels/RenderingSettingsPanel.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Style/SparkleUiPalette.h"

#include <imgui.h>

#include <string>

namespace
{
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

void RenderingSettingsPanel::BuildUI(bool disableInteraction)
{
	if (m_settings == nullptr)
	{
		return;
	}

	const EngineRenderingSettingsState& settings = m_settings->GetState();

	ImGui::TextUnformatted("Engine - Rendering");
	ImGui::Spacing();

	if (m_settings->HasPendingRestart())
	{
		const std::string restartMessage = m_settings->BuildPendingRestartMessage();
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::AccentStrong());
		ImGui::TextWrapped("%s", restartMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Spacing();
	}

	ImGui::BeginDisabled(disableInteraction);

	bool vsync = settings.VSync;
	if (ImGui::Checkbox("VSync", &vsync))
	{
		m_settings->SetVSync(vsync);
	}

	bool highPerformanceAdapter = settings.PreferHighPerformanceAdapter;
	if (ImGui::Checkbox("Prefer high-performance adapter", &highPerformanceAdapter))
	{
		m_settings->SetPreferHighPerformanceAdapter(highPerformanceAdapter);
	}

	bool meshAutoBatching = settings.MeshAutoBatching;
	if (ImGui::Checkbox("Mesh auto batching", &meshAutoBatching))
	{
		m_settings->SetMeshAutoBatching(meshAutoBatching);
	}

	static constexpr const char* tlasModeLabels[] = {"Classic TLAS", "Partitioned TLAS"};
	int tlasModeIndex = ToTlasModeIndex(settings.RayTracingTopLevelMode);
	if (ImGui::Combo("Ray tracing TLAS", &tlasModeIndex, tlasModeLabels, IM_ARRAYSIZE(tlasModeLabels)))
	{
		m_settings->SetRayTracingTopLevelMode(FromTlasModeIndex(tlasModeIndex));
	}

	static constexpr const char* ptlasPathLabels[] = {
	    "CPU pack",
	    "GPU dirty + CPU native pack",
	    "Full GPU native pack",
	};
	int ptlasPathIndex = ToPtlasUpdatePathIndex(settings.PtlasUpdatePath);
	if (ImGui::Combo("PTLAS update path", &ptlasPathIndex, ptlasPathLabels, IM_ARRAYSIZE(ptlasPathLabels)))
	{
		m_settings->SetPtlasUpdatePath(FromPtlasUpdatePathIndex(ptlasPathIndex));
	}

	ImGui::EndDisabled();
}
