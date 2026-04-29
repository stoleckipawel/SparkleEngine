#pragma once

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <imgui.h>

// Renders the donut + bar chart pair that accompanies a profiler tree-table.
// Owns no persistent state; the caller passes in the bucket of nodes to chart
// plus the set of scope names the user has hidden.
class ProfilerChartView
{
  public:
	ProfilerChartView() = default;
	~ProfilerChartView() = default;

	ProfilerChartView(const ProfilerChartView&) = delete;
	ProfilerChartView& operator=(const ProfilerChartView&) = delete;

	// Renders charts for `bucket`. `moduleName` becomes the panel ID and is also
	// used to disambiguate ImGui IDs when multiple charts appear on the page.
	// Returns false if there isn't enough room or there are no visible slices.
	bool Render(
	    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
	    std::string_view moduleName,
	    const std::unordered_set<std::string>& hiddenScopes) const;

  private:
	// Per-slice data prepared up front so layout, hit-testing and rendering
	// share the same numbers. Keeping it here (instead of an anonymous helper)
	// keeps the chart's data shape discoverable from the header.
	struct Slice
	{
		std::string ShortName;
		double ValueMs = 0.0;
		double Pct = 0.0;
		ImU32 Color = 0;
	};

	// ---- Geometry constants ----
	// Centralized so a future tweak doesn't require hunting through the .cpp.
	static constexpr float kPi = 3.14159265358979323846f;
	static constexpr float kOuterPadding = 10.0f;
	static constexpr float kInnerPadding = 14.0f;
	static constexpr float kPiePanelMaxWidth = 200.0f;
	static constexpr float kPieWidthFraction = 0.33f;
	static constexpr float kPieMaxDiameter = 130.0f;
	static constexpr float kDonutHoleFraction = 0.55f;
	static constexpr float kBarMaxWidth = 40.0f;
	static constexpr float kBarMinSpacing = 6.0f;
	static constexpr float kBarMinWidth = 8.0f;
	static constexpr float kMinChartWidth = 260.0f;
	static constexpr float kMinPlotWidth = 60.0f;
	static constexpr float kLabelAngleDegrees = 35.0f;
	static constexpr float kRowTintSaturation = 0.85f;

	// Steps used by the y-axis. Matches grid + tick label loop.
	static constexpr int kYAxisGridSteps = 4;

	// Named so the magic isn't repeated literal-by-literal in the .cpp.
	static constexpr float kCardCornerRadius = 4.0f;
	static constexpr float kBarCornerRadius = 1.5f;

	// Rounds a value up to a "nice" axis maximum (1, 2, 5, 10 Ă— 10^n).
	static double NiceCeil(double value) noexcept;

	// Build slice list from `bucket`, skipping hidden scopes and reducing parent
	// slices when hidden descendants are nested below them.
	// Returns true if at least one slice has a positive value.
	static bool BuildSlices(
	    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
	    const std::unordered_set<std::string>& hiddenScopes,
	    std::vector<Slice>& outSlices,
	    double& outTotalMs,
	    double& outMaxMs);

	static bool IsScopeHidden(const Diagnostics::ProfilerSnapshotNode& node, const std::unordered_set<std::string>& hiddenScopes) noexcept;
	static double ComputeHiddenDescendantAverageMicroseconds(
	    const Diagnostics::ProfilerSnapshotNode& node,
	    const std::unordered_set<std::string>& hiddenScopes) noexcept;
	static double ComputeVisibleAverageMicroseconds(
	    const Diagnostics::ProfilerSnapshotNode& node,
	    const std::unordered_set<std::string>& hiddenScopes) noexcept;
};
