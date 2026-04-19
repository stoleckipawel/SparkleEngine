#include "PCH.h"
#include "Panels/ProfilerPanel.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>

#include <imgui.h>

namespace
{
	constexpr ImGuiTableFlags kTableFlags =
	    ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

	constexpr double kMicrosecondsToMilliseconds = 1.0 / 1000.0;

	void FormatThreadLabel(char* buffer, std::size_t bufferSize, const Engine::Diagnostics::ProfilerThreadSnapshot& thread)
	{
		if (!thread.ThreadName.empty())
		{
			std::snprintf(buffer, bufferSize, "%s (TID %u)", thread.ThreadName.c_str(), thread.ThreadId);
		}
		else
		{
			std::snprintf(buffer, bufferSize, "Thread %u", thread.ThreadId);
		}
	}
}

void ProfilerPanel::BuildUI(bool /*disableInteraction*/)
{
	if (!m_isVisible)
	{
		return;
	}

	Engine::Diagnostics::LiveProfiler& profiler = Engine::Diagnostics::LiveProfiler::Get();
	m_snapshot = profiler.CaptureSnapshot();

	ImGuiIO& io = ImGui::GetIO();
	const float defaultX = (std::max) (10.0f, io.DisplaySize.x - m_widthPixels - 10.0f);
	const float defaultY = m_topInsetPixels + 10.0f;

	ImGui::SetNextWindowPos(ImVec2(defaultX, defaultY), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, m_heightPixels), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Profiler", &m_isVisible))
	{
		ImGui::End();
		return;
	}

	RenderToolbar(profiler);

	if (ImGui::BeginTabBar("##ProfilerTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("CPU"))
		{
			RenderCpuTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("GPU"))
		{
			RenderGpuTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void ProfilerPanel::RenderToolbar(Engine::Diagnostics::LiveProfiler& profiler) noexcept
{
	bool enabled = profiler.IsEnabled();
	if (ImGui::Checkbox("Capturing", &enabled))
	{
		profiler.SetEnabled(enabled);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset"))
	{
		profiler.Reset();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("avg uses EMA (alpha = 0.1)");
	ImGui::Separator();
}

void ProfilerPanel::RenderCpuTab() const
{
	if (m_snapshot.CpuThreads.empty())
	{
		ImGui::TextDisabled("No CPU samples captured yet.");
		return;
	}

	for (const Engine::Diagnostics::ProfilerThreadSnapshot& thread : m_snapshot.CpuThreads)
	{
		char threadLabel[96] = {};
		FormatThreadLabel(threadLabel, sizeof(threadLabel), thread);

		const ImGuiTreeNodeFlags threadFlags =
		    ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (!ImGui::CollapsingHeader(threadLabel, threadFlags))
		{
			continue;
		}

		ImGui::PushID(static_cast<int>(thread.ThreadId));
		if (ImGui::BeginTable("##CpuTable", 4, kTableFlags))
		{
			ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableHeadersRow();

			for (const Engine::Diagnostics::ProfilerSnapshotNode& root : thread.Roots)
			{
				RenderNodeRow(root);
			}

			ImGui::EndTable();
		}
		ImGui::PopID();
	}
}

void ProfilerPanel::RenderGpuTab() const
{
	if (m_snapshot.GpuRoots.empty())
	{
		ImGui::TextDisabled("No GPU samples captured yet.");
		return;
	}

	if (ImGui::BeginTable("##GpuTable", 4, kTableFlags))
	{
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableHeadersRow();

		for (const Engine::Diagnostics::ProfilerSnapshotNode& root : m_snapshot.GpuRoots)
		{
			RenderNodeRow(root);
		}

		ImGui::EndTable();
	}
}

void ProfilerPanel::RenderNodeRow(const Engine::Diagnostics::ProfilerSnapshotNode& node)
{
	ImGui::TableNextRow();

	ImGui::TableSetColumnIndex(0);
	const bool hasChildren = !node.Children.empty();
	const ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAllColumns
	                                     | ImGuiTreeNodeFlags_DefaultOpen
	                                     | (hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
	const bool open = ImGui::TreeNodeEx(node.Name.c_str(), nodeFlags);

	ImGui::TableSetColumnIndex(1);
	ImGui::Text("%.3f", node.LastDurationMicroseconds * kMicrosecondsToMilliseconds);
	ImGui::TableSetColumnIndex(2);
	ImGui::Text("%.3f", node.AverageDurationMicroseconds * kMicrosecondsToMilliseconds);
	ImGui::TableSetColumnIndex(3);
	ImGui::Text("%" PRIu64, node.TotalCallCount);

	if (open)
	{
		for (const Engine::Diagnostics::ProfilerSnapshotNode& child : node.Children)
		{
			RenderNodeRow(child);
		}
		ImGui::TreePop();
	}
}
