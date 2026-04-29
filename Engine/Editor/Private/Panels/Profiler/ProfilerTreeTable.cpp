#include "PCH.h"
#include "Panels/Profiler/ProfilerTreeTable.h"

#include "Panels/Profiler/ProfilerSnapshotUtils.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <string>

#include <imgui.h>

void ProfilerTreeTable::RightAlignedText(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[64];
	std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	const float textW = ImGui::CalcTextSize(buf).x;
	const float colW = ImGui::GetContentRegionAvail().x;
	if (colW > textW)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colW - textW));
	}
	ImGui::TextUnformatted(buf);
}

void ProfilerTreeTable::Render(const State& state, const char* tableId, const std::vector<const Diagnostics::ProfilerSnapshotNode*>& nodes)
    const
{
	BeginTable(tableId);
	RenderRows(state, nodes, 0, 0, nodes.size());
	EndTable();
}

void ProfilerTreeTable::BeginTable(const char* tableId) const
{
	ImGui::PushID(tableId);
	if (ImGui::BeginTable("##Table", 6, kTableFlags))
	{
		ImGui::TableSetupColumn(
		    "##Vis",
		    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_IndentDisable,
		    kVisibilityColumnWidth);
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentEnable);
		ImGui::TableSetupColumn("Incl ms", ImGuiTableColumnFlags_WidthFixed, kNumericColumnWidth);
		ImGui::TableSetupColumn("Excl ms", ImGuiTableColumnFlags_WidthFixed, kNumericColumnWidth);
		ImGui::TableSetupColumn("Max ms", ImGuiTableColumnFlags_WidthFixed, kNumericColumnWidth);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, kCallsColumnWidth);
		ImGui::TableHeadersRow();
	}
}

void ProfilerTreeTable::EndTable() const
{
	ImGui::EndTable();
	ImGui::PopID();
}

void ProfilerTreeTable::RenderRows(
    const State& state,
    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& nodes,
    int depth,
    std::size_t siblingOffset,
    std::size_t siblingTotal) const
{
	const std::size_t total = siblingTotal > 0 ? siblingTotal : nodes.size();
	for (std::size_t i = 0; i < nodes.size(); ++i)
	{
		RenderRow(state, *nodes[i], depth, siblingOffset + i, total);
	}
}

void ProfilerTreeTable::RenderRow(
    const State& state,
    const Diagnostics::ProfilerSnapshotNode& node,
    int depth,
    std::size_t siblingIndex,
    std::size_t siblingTotal) const
{
	ImGui::TableNextRow();

	// Tint the row background with a low-opacity color matching the chart slice.
	if (siblingTotal > 0)
	{
		const ImU32 base = SparkleUiPalette::CategoricalColorDesaturated(siblingIndex, kRowTintSaturation);
		const ImU32 tint = (base & 0x00FFFFFFu) | (kRowTintAlpha << 24);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, tint);
	}

	ImGui::TableSetColumnIndex(0);
	if (depth == 0)
	{
		RenderVisibilityToggle(state, node);
	}

	ImGui::TableSetColumnIndex(1);
	const bool hasChildren = !node.Children.empty();
	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow |
	                               (hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
	if (depth == 0)
	{
		nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	const std::string_view displayName = ProfilerSnapshotUtils::ShortenScopeName(node.Name);
	const std::string displayNameStr(displayName);
	const bool open = ImGui::TreeNodeEx(displayNameStr.c_str(), nodeFlags);

	// Click anywhere on the label (but not the arrow) to focus charts on this scope.
	if (hasChildren && state.ChartFocusNodeName != nullptr && ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		*state.ChartFocusNodeName = node.Name;
	}

	const auto exclusive = ProfilerSnapshotUtils::ComputeExclusiveWithStatus(node);
	const double inclusiveMs = ProfilerSnapshotUtils::MicrosecondsToMilliseconds(exclusive.InclusiveMicroseconds);
	const double exclusiveMs = ProfilerSnapshotUtils::MicrosecondsToMilliseconds(exclusive.ExclusiveMicroseconds);
	const double maxMs = ProfilerSnapshotUtils::MicrosecondsToMilliseconds(node.MaxDurationMicroseconds);

	const bool hasDrawStats = node.DrawCallCount > 0 || node.DispatchCount > 0;
	if (hasDrawStats && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		RenderDrawStatsTooltip(node);
	}

	ImGui::TableSetColumnIndex(2);
	RightAlignedText("%.3f", inclusiveMs);
	ImGui::TableSetColumnIndex(3);
	if (exclusive.WasClampedToZero)
	{
		// Visually flag scopes whose children's measured time exceeded the
		// parent's â€” typically async/overlapping work where exclusive time is
		// not meaningful.
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
		RightAlignedText("~%.3f", exclusiveMs);
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip(
			    "Exclusive time was clamped to zero because measured child time\n"
			    "(%.3f ms) exceeded inclusive time (%.3f ms).\n"
			    "This usually indicates async or overlapping child scopes.",
			    ProfilerSnapshotUtils::MicrosecondsToMilliseconds(exclusive.ChildSumMicroseconds),
			    inclusiveMs);
		}
	}
	else
	{
		RightAlignedText("%.3f", exclusiveMs);
	}
	ImGui::TableSetColumnIndex(4);
	RightAlignedText("%.3f", maxMs);
	ImGui::TableSetColumnIndex(5);
	if (hasDrawStats)
	{
		if (node.DispatchCount > 0)
		{
			RightAlignedText("%" PRIu64 " (%" PRIu64 "d/%" PRIu64 "x)", node.TotalCallCount, node.DrawCallCount, node.DispatchCount);
		}
		else
		{
			RightAlignedText("%" PRIu64 " (%" PRIu64 "d)", node.TotalCallCount, node.DrawCallCount);
		}
	}
	else
	{
		RightAlignedText("%" PRIu64, node.TotalCallCount);
	}

	if (open)
	{
		std::vector<const Diagnostics::ProfilerSnapshotNode*> childPtrs;
		childPtrs.reserve(node.Children.size());
		for (const Diagnostics::ProfilerSnapshotNode& child : node.Children)
		{
			childPtrs.push_back(&child);
		}
		RenderRows(state, childPtrs, depth + 1, 0, 0);
		ImGui::TreePop();
	}
}

void ProfilerTreeTable::RenderVisibilityToggle(const State& state, const Diagnostics::ProfilerSnapshotNode& node) const
{
	if (state.HiddenScopes == nullptr)
	{
		return;
	}

	const bool isHidden = state.HiddenScopes->count(node.Name) > 0;
	ImGui::PushID(node.Name.c_str());

	const float contentW = ImGui::GetContentRegionAvail().x;
	constexpr float buttonSize = 14.0f;
	const float offset = (std::max) (0.0f, (contentW - buttonSize) * 0.5f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	if (UiUtil::DrawVisibilityIconButton("chart_visibility", !isHidden))
	{
		if (isHidden)
		{
			state.HiddenScopes->erase(node.Name);
		}
		else
		{
			state.HiddenScopes->insert(node.Name);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(isHidden ? "Show in charts" : "Hide from charts");
	}
	ImGui::PopID();
}

void ProfilerTreeTable::RenderDrawStatsTooltip(const Diagnostics::ProfilerSnapshotNode& node) const
{
	const double invocations = node.TotalCallCount > 0 ? static_cast<double>(node.TotalCallCount) : 1.0;
	ImGui::BeginTooltip();
	ImGui::TextUnformatted(node.Name.c_str());
	ImGui::Separator();
	if (node.DrawCallCount > 0)
	{
		const std::uint64_t nonIndexed = node.DrawCallCount - node.IndexedDrawCount;
		ImGui::Text(
		    "Draws       %" PRIu64 "  (avg %.1f / invocation)",
		    node.DrawCallCount,
		    static_cast<double>(node.DrawCallCount) / invocations);
		ImGui::Text("  Indexed   %" PRIu64, node.IndexedDrawCount);
		ImGui::Text("  NonIndexed %" PRIu64, nonIndexed);
		ImGui::Text(
		    "Vertices    %" PRIu64 "  (avg %.0f / invocation)",
		    node.TotalVertexCount,
		    static_cast<double>(node.TotalVertexCount) / invocations);
		ImGui::Text(
		    "Instances   %" PRIu64 "  (avg %.1f / invocation)",
		    node.TotalInstanceCount,
		    static_cast<double>(node.TotalInstanceCount) / invocations);
	}
	if (node.DispatchCount > 0)
	{
		ImGui::Text(
		    "Dispatches  %" PRIu64 "  (avg %.1f / invocation)",
		    node.DispatchCount,
		    static_cast<double>(node.DispatchCount) / invocations);
		ImGui::Text("ThreadGrps  %" PRIu64, node.TotalThreadGroupCount);
	}
	ImGui::EndTooltip();
}
