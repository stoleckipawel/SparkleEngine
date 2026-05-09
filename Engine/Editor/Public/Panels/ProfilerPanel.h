#pragma once

#include "../EditorAPI.h"
#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Panels/Profiler/ProfilerChartView.h"
#include "Panels/Profiler/ProfilerSorting.h"
#include "Panels/Profiler/ProfilerTreeTable.h"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// Embedded profiler panel (CPU + GPU). The panel itself is a thin orchestrator:
// the heavy lifting lives in the Panels/Profiler/ helper modules so each piece
// can be reasoned about â€” and one day tested â€” in isolation.
class SPARKLE_EDITOR_API ProfilerPanel final
{
  public:
	ProfilerPanel() noexcept = default;
	~ProfilerPanel() = default;

	ProfilerPanel(const ProfilerPanel&) = delete;
	ProfilerPanel(ProfilerPanel&&) = delete;
	ProfilerPanel& operator=(const ProfilerPanel&) = delete;
	ProfilerPanel& operator=(ProfilerPanel&&) = delete;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }

	// Captures a fresh snapshot from the LiveProfiler and renders the panel
	// inside the current ImGui window. Pass `disableInteraction=true` to gray
	// out controls without hiding the data (e.g. while paused).
	void BuildUI(bool disableInteraction = false);
	void BuildEmbeddedUI(bool disableInteraction = false);

  private:
	void RenderToolbar() noexcept;
	void RenderCpuTab() const;
	void RenderGpuTab() const;
	void RenderGroupedNodes(const std::vector<Diagnostics::ProfilerSnapshotNode>& nodes) const;
	void RenderChartsForBucket(const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket, std::string_view defaultModuleName)
	    const;
	ProfilerTreeTable::State MakeTableState() const noexcept;

	Diagnostics::ProfilerSnapshot m_snapshot;
	ProfilerSorting::SortMode m_sortMode = ProfilerSorting::SortMode::Hierarchy;

	// Mutated from `const` rendering paths; UI-owned state, so `mutable` keeps
	// Renderâ€¦ methods const.
	mutable std::unordered_set<std::string> m_hiddenScopes;
	mutable std::string m_chartFocusNodeName;

	ProfilerTreeTable m_treeTable;
	ProfilerChartView m_chartView;
	bool m_isOpen = false;
};
