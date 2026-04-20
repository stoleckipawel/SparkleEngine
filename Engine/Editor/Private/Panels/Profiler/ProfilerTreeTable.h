#pragma once

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

#include <imgui.h>

class ProfilerTreeTable
{
  public:
	// State the table needs from the owner. Owned externally so the table itself
	// stays free of persistent state — this keeps it easy to test and reuse.
	struct State
	{
		// Names of root-level scopes the user has hidden from the chart.
		std::unordered_set<std::string>* HiddenScopes = nullptr;
		// Set when the user clicks a row label so the chart can drill in.
		// Cleared by the table when the same row is clicked twice (toggle behavior
		// is owned by the caller; the table only writes the clicked node name).
		std::string* ChartFocusNodeName = nullptr;
	};

	ProfilerTreeTable() = default;
	~ProfilerTreeTable() = default;

	ProfilerTreeTable(const ProfilerTreeTable&) = delete;
	ProfilerTreeTable& operator=(const ProfilerTreeTable&) = delete;

	// Renders an ImGui table containing the supplied flat node bucket. Recurses
	// into children when the user expands tree rows.
	void Render(
	    const State& state,
	    const char* tableId,
	    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& nodes) const;

  private:
	// ImGui table flags shared by every profiler table instance.
	static constexpr ImGuiTableFlags kTableFlags =
	    ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

	static constexpr float kVisibilityColumnWidth = 20.0f;
	static constexpr float kNumericColumnWidth = 70.0f;
	static constexpr float kCallsColumnWidth = 64.0f;

	// Alpha applied to row tint (0..255). Subtle hint that ties rows back to chart slice colors.
	static constexpr unsigned int kRowTintAlpha = 0x19u;
	static constexpr float kRowTintSaturation = 0.85f;

	void BeginTable(const char* tableId) const;
	void EndTable() const;

	void RenderRows(
	    const State& state,
	    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& nodes,
	    int depth,
	    std::size_t siblingOffset,
	    std::size_t siblingTotal) const;

	void RenderRow(
	    const State& state,
	    const Engine::Diagnostics::ProfilerSnapshotNode& node,
	    int depth,
	    std::size_t siblingIndex,
	    std::size_t siblingTotal) const;

	void RenderVisibilityDot(
	    const State& state,
	    const Engine::Diagnostics::ProfilerSnapshotNode& node,
	    std::size_t siblingIndex,
	    std::size_t siblingTotal) const;

	void RenderDrawStatsTooltip(const Engine::Diagnostics::ProfilerSnapshotNode& node) const;

	static void RightAlignedText(const char* fmt, ...);
};
