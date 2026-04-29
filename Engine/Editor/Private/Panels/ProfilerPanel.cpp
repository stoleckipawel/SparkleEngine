#include "PCH.h"
#include "Panels/ProfilerPanel.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Panels/Profiler/ProfilerSnapshotUtils.h"
#include "Util/UiUtil.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <imgui.h>

void ProfilerPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(1080.0f, 640.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Profiler, "Profiler") + "##Profiler";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	BuildEmbeddedUI(disableInteraction);
	ImGui::End();
}

void ProfilerPanel::BuildEmbeddedUI(bool disableInteraction)
{
	Diagnostics::LiveProfiler& profiler = Diagnostics::LiveProfiler::Get();
	m_snapshot = profiler.CaptureSnapshot();

	ImGui::BeginDisabled(disableInteraction);
	RenderToolbar();

	if (ImGui::BeginTabBar("##ProfilerTabs", ImGuiTabBarFlags_None))
	{
		const std::string cpuLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Cpu, "CPU");
		if (ImGui::BeginTabItem(cpuLabel.c_str()))
		{
			ImGui::BeginChild("##CpuScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
			RenderCpuTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		const std::string gpuLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Gpu, "GPU");
		if (ImGui::BeginTabItem(gpuLabel.c_str()))
		{
			ImGui::BeginChild("##GpuScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
			RenderGpuTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::EndDisabled();
}

ProfilerTreeTable::State ProfilerPanel::MakeTableState() const noexcept
{
	ProfilerTreeTable::State s;
	s.HiddenScopes = &m_hiddenScopes;
	s.ChartFocusNodeName = &m_chartFocusNodeName;
	return s;
}

void ProfilerPanel::RenderToolbar() noexcept
{
	// Toolbar row: "Sort:" label + dropdown on the left, frame summary on the right.
	ImGui::AlignTextToFramePadding();
	const std::string sortLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Sort, "SORT");
	ImGui::TextDisabled("%s", sortLabel.c_str());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(170.0f);
	int sortIndex = static_cast<int>(m_sortMode);
	if (ImGui::Combo("##SortCombo", &sortIndex, ProfilerSorting::SortModeLabels(), ProfilerSorting::SortModeCount()))
	{
		m_sortMode = static_cast<ProfilerSorting::SortMode>(sortIndex);
	}

	// Right-aligned summary: thread + GPU pass counts. We collapse a single GPU
	// "Frame" root into its children so the count matches what the table shows.
	const std::size_t gpuPassCount = !m_snapshot.GpuRoots.empty() && m_snapshot.GpuRoots.size() == 1
	    ? m_snapshot.GpuRoots[0].Children.size()
	    : m_snapshot.GpuRoots.size();

	char summaryBuf[96];
	std::snprintf(
	    summaryBuf,
	    sizeof(summaryBuf),
	    "CPU threads: %zu  \xC2\xB7  GPU passes: %zu",
	    m_snapshot.CpuThreads.size(),
	    gpuPassCount);
	const float summaryW = ImGui::CalcTextSize(summaryBuf).x;
	const float availX = ImGui::GetContentRegionAvail().x;
	if (availX > summaryW + 12.0f)
	{
		ImGui::SameLine(0.0f, availX - summaryW - 4.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", summaryBuf);
	}

	ImGui::Separator();
}

void ProfilerPanel::RenderCpuTab() const
{
	if (m_snapshot.CpuThreads.empty())
	{
		ImGui::TextDisabled("No CPU samples captured yet.");
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	for (const Diagnostics::ProfilerThreadSnapshot& thread : m_snapshot.CpuThreads)
	{
		char threadLabel[96] = {};
		ProfilerSnapshotUtils::FormatThreadLabel(threadLabel, sizeof(threadLabel), thread);

		const ImGuiTreeNodeFlags threadFlags =
		    ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (!ImGui::CollapsingHeader(threadLabel, threadFlags))
		{
			continue;
		}

		ImGui::PushID(static_cast<int>(thread.ThreadId));
		RenderGroupedNodes(thread.Roots);
		ImGui::PopID();
	}
	ImGui::PopStyleVar();
}

void ProfilerPanel::RenderGpuTab() const
{
	if (m_snapshot.GpuRoots.empty())
	{
		ImGui::TextDisabled("No GPU samples captured yet.");
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	std::vector<const Diagnostics::ProfilerSnapshotNode*> bucket;
	bucket.reserve(m_snapshot.GpuRoots.size());
	for (const Diagnostics::ProfilerSnapshotNode& node : m_snapshot.GpuRoots)
	{
		bucket.push_back(&node);
	}
	ProfilerSorting::SortBucket(bucket, m_sortMode);
	m_treeTable.Render(MakeTableState(), "gpu", bucket);

	// Charts: focused-children if the user clicked a row, otherwise auto-drill
	// down through any single-child wrappers (the "GPU Frame" root) so we land
	// on the actual passes.
	const Diagnostics::ProfilerSnapshotNode* gpuFocus = m_chartFocusNodeName.empty()
	    ? nullptr
	    : ProfilerSnapshotUtils::FindNodeByName(m_snapshot.GpuRoots, m_chartFocusNodeName);
	if (gpuFocus != nullptr && m_hiddenScopes.count(gpuFocus->Name) == 0 && gpuFocus->Children.size() >= 2)
	{
		std::vector<const Diagnostics::ProfilerSnapshotNode*> chartBucket;
		chartBucket.reserve(gpuFocus->Children.size());
		for (const Diagnostics::ProfilerSnapshotNode& child : gpuFocus->Children)
		{
			chartBucket.push_back(&child);
		}
		m_chartView.Render(chartBucket, ProfilerSnapshotUtils::ShortenScopeName(gpuFocus->Name), m_hiddenScopes);
	}
	else
	{
		const std::vector<Diagnostics::ProfilerSnapshotNode>* chartLevel = &m_snapshot.GpuRoots;
		bool hiddenAutoDrillAncestor = false;
		while (chartLevel->size() == 1 && !(*chartLevel)[0].Children.empty())
		{
			hiddenAutoDrillAncestor = hiddenAutoDrillAncestor || m_hiddenScopes.count((*chartLevel)[0].Name) > 0;
			chartLevel = &(*chartLevel)[0].Children;
		}
		if (!hiddenAutoDrillAncestor && chartLevel->size() >= 2)
		{
			std::vector<const Diagnostics::ProfilerSnapshotNode*> chartBucket;
			chartBucket.reserve(chartLevel->size());
			for (const Diagnostics::ProfilerSnapshotNode& node : *chartLevel)
			{
				chartBucket.push_back(&node);
			}
			m_chartView.Render(chartBucket, "GPU", m_hiddenScopes);
		}
	}
	ImGui::PopStyleVar();
}

void ProfilerPanel::RenderGroupedNodes(const std::vector<Diagnostics::ProfilerSnapshotNode>& nodes) const
{
	if (nodes.empty())
	{
		return;
	}

	// Build module groups, preserving first-seen order so the UI stays stable
	// across captures even though `unordered_map` iteration is not.
	std::vector<std::string_view> moduleOrder;
	std::unordered_map<std::string_view, std::vector<const Diagnostics::ProfilerSnapshotNode*>> grouped;
	for (const Diagnostics::ProfilerSnapshotNode& node : nodes)
	{
		const std::string_view moduleName = ProfilerSnapshotUtils::ExtractModuleName(node.Name);
		auto [it, inserted] = grouped.try_emplace(moduleName);
		if (inserted)
		{
			moduleOrder.push_back(moduleName);
		}
		it->second.push_back(&node);
	}

	// Single module â€” no grouping headers, just one table + chart pair.
	if (moduleOrder.size() < 2)
	{
		std::vector<const Diagnostics::ProfilerSnapshotNode*> bucket;
		bucket.reserve(nodes.size());
		for (const Diagnostics::ProfilerSnapshotNode& node : nodes)
		{
			bucket.push_back(&node);
		}
		ProfilerSorting::SortBucket(bucket, m_sortMode);
		m_treeTable.Render(MakeTableState(), "ungrouped", bucket);
		RenderChartsForBucket(bucket, moduleOrder.empty() ? std::string_view{} : moduleOrder.front());
		return;
	}

	// Multiple modules â€” collapsing header + table + chart per group.
	for (std::string_view moduleName : moduleOrder)
	{
		auto& bucket = grouped[moduleName];
		ProfilerSorting::SortBucket(bucket, m_sortMode);

		std::string headerLabel{"["};
		headerLabel.append(moduleName.begin(), moduleName.end());
		headerLabel += "]";

		const ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		ImGui::PushID(headerLabel.c_str());
		const bool groupOpen = ImGui::CollapsingHeader(headerLabel.c_str(), groupFlags);
		if (groupOpen)
		{
			m_treeTable.Render(MakeTableState(), headerLabel.c_str(), bucket);
			RenderChartsForBucket(bucket, moduleName);
		}
		ImGui::PopID();
	}
}

void ProfilerPanel::RenderChartsForBucket(
    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
    std::string_view defaultModuleName) const
{
	// Drill into the focused node's children when the user clicked a row that
	// belongs to *this* bucket. Anything else falls back to the bucket itself.
	const Diagnostics::ProfilerSnapshotNode* focusNode = m_chartFocusNodeName.empty()
	    ? nullptr
	    : ProfilerSnapshotUtils::FindNodeInBucket(bucket, m_chartFocusNodeName);
	if (focusNode != nullptr && m_hiddenScopes.count(focusNode->Name) == 0 && focusNode->Children.size() >= 2)
	{
		std::vector<const Diagnostics::ProfilerSnapshotNode*> chartBucket;
		chartBucket.reserve(focusNode->Children.size());
		for (const Diagnostics::ProfilerSnapshotNode& child : focusNode->Children)
		{
			chartBucket.push_back(&child);
		}
		m_chartView.Render(chartBucket, ProfilerSnapshotUtils::ShortenScopeName(focusNode->Name), m_hiddenScopes);
		return;
	}
	if (bucket.size() >= 2)
	{
		m_chartView.Render(bucket, defaultModuleName, m_hiddenScopes);
	}
}
