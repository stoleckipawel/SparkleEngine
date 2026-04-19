#include "PCH.h"
#include "Panels/ProfilerPanel.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

	double SumChildAverages(const std::vector<Engine::Diagnostics::ProfilerSnapshotNode>& children) noexcept
	{
		double sum = 0.0;
		for (const Engine::Diagnostics::ProfilerSnapshotNode& child : children)
		{
			sum += child.AverageDurationMicroseconds;
		}
		return sum;
	}

	std::string_view ExtractModuleName(std::string_view scopeName) noexcept
	{
		// CPU scopes use dotted names like "Renderer.RecordFrame". Module is the first segment.
		if (const std::size_t dot = scopeName.find('.'); dot != std::string_view::npos)
		{
			return scopeName.substr(0, dot);
		}
		// GPU scopes use "[Kind #N] PassName". Module is the kind inside brackets.
		if (!scopeName.empty() && scopeName.front() == '[')
		{
			const std::size_t space = scopeName.find(' ');
			if (space != std::string_view::npos && space > 1)
			{
				return scopeName.substr(1, space - 1);
			}
		}
		return scopeName;
	}
}

void ProfilerPanel::BuildEmbeddedUI(bool disableInteraction)
{
	Engine::Diagnostics::LiveProfiler& profiler = Engine::Diagnostics::LiveProfiler::Get();
	m_snapshot = profiler.CaptureSnapshot();

	ImGui::BeginDisabled(disableInteraction);
	RenderToolbar(profiler);

	if (ImGui::BeginTabBar("##ProfilerTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("CPU"))
		{
			ImGui::BeginChild("##CpuScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
			RenderCpuTab();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("GPU"))
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

void ProfilerPanel::RenderToolbar(Engine::Diagnostics::LiveProfiler& profiler) noexcept
{
	static constexpr const char* kSortLabels[] = {
	    "Hierarchy (grouped)",
	    "Name (A-Z)",
	    "Name (Z-A)",
	    "Incl (high-low)",
	    "Excl (high-low)",
	    "Max (high-low)",
	    "Calls (high-low)",
	};
	ImGui::SetNextItemWidth(160.0f);
	int sortIndex = static_cast<int>(m_sortMode);
	if (ImGui::Combo("Sort", &sortIndex, kSortLabels, IM_ARRAYSIZE(kSortLabels)))
	{
		m_sortMode = static_cast<SortMode>(sortIndex);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Charts", &m_showCharts);
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
		RenderGroupedNodes(thread.Roots);
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

	// Flat list — no module grouping for GPU.
	std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*> bucket;
	bucket.reserve(m_snapshot.GpuRoots.size());
	for (const Engine::Diagnostics::ProfilerSnapshotNode& node : m_snapshot.GpuRoots)
	{
		bucket.push_back(&node);
	}
	SortBucket(bucket);
	BeginProfilerTable("gpu");
	RenderTableRows(bucket, 0);
	ImGui::EndTable();
	ImGui::PopID();

	// Chart the pass-level nodes. If there's a single root, use its children.
	if (m_showCharts)
	{
		std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*> chartBucket;
		if (bucket.size() == 1 && !bucket[0]->Children.empty())
		{
			chartBucket.reserve(bucket[0]->Children.size());
			for (const Engine::Diagnostics::ProfilerSnapshotNode& child : bucket[0]->Children)
			{
				chartBucket.push_back(&child);
			}
		}
		else
		{
			chartBucket = bucket;
		}
		if (chartBucket.size() >= 2)
		{
			RenderModuleCharts(chartBucket, "GPU");
		}
	}
}

void ProfilerPanel::RenderNodeRow(const Engine::Diagnostics::ProfilerSnapshotNode& node, int depth) const
{
	ImGui::TableNextRow();

	ImGui::TableSetColumnIndex(0);
	const bool hasChildren = !node.Children.empty();
	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAllColumns
	                               | (hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
	if (depth == 0)
	{
		nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	const bool open = ImGui::TreeNodeEx(node.Name.c_str(), nodeFlags);

	const double inclusiveMs = node.AverageDurationMicroseconds * kMicrosecondsToMilliseconds;
	const double childSumMs = SumChildAverages(node.Children) * kMicrosecondsToMilliseconds;
	const double exclusiveMs = std::max(0.0, inclusiveMs - childSumMs);

	const bool hasDrawStats = node.DrawCallCount > 0 || node.DispatchCount > 0;
	if (hasDrawStats && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		const double frames = node.TotalCallCount > 0 ? static_cast<double>(node.TotalCallCount) : 1.0;
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(node.Name.c_str());
		ImGui::Separator();
		if (node.DrawCallCount > 0)
		{
			const std::uint64_t nonIndexed = node.DrawCallCount - node.IndexedDrawCount;
			ImGui::Text("Draws       %" PRIu64 "  (avg %.1f / scope)", node.DrawCallCount, static_cast<double>(node.DrawCallCount) / frames);
			ImGui::Text("  Indexed   %" PRIu64, node.IndexedDrawCount);
			ImGui::Text("  NonIndexed %" PRIu64, nonIndexed);
			ImGui::Text("Vertices    %" PRIu64 "  (avg %.0f / scope)", node.TotalVertexCount, static_cast<double>(node.TotalVertexCount) / frames);
			ImGui::Text("Instances   %" PRIu64 "  (avg %.1f / scope)", node.TotalInstanceCount, static_cast<double>(node.TotalInstanceCount) / frames);
		}
		if (node.DispatchCount > 0)
		{
			ImGui::Text("Dispatches  %" PRIu64 "  (avg %.1f / scope)", node.DispatchCount, static_cast<double>(node.DispatchCount) / frames);
			ImGui::Text("ThreadGrps  %" PRIu64, node.TotalThreadGroupCount);
		}
		ImGui::EndTooltip();
	}

	ImGui::TableSetColumnIndex(1);
	ImGui::Text("%.3f", inclusiveMs);
	ImGui::TableSetColumnIndex(2);
	ImGui::Text("%.3f", exclusiveMs);
	ImGui::TableSetColumnIndex(3);
	ImGui::Text("%.3f", node.MaxDurationMicroseconds * kMicrosecondsToMilliseconds);
	ImGui::TableSetColumnIndex(4);
	if (hasDrawStats)
	{
		if (node.DispatchCount > 0)
		{
			ImGui::Text("%" PRIu64 " (%" PRIu64 "d/%" PRIu64 "x)", node.TotalCallCount, node.DrawCallCount, node.DispatchCount);
		}
		else
		{
			ImGui::Text("%" PRIu64 " (%" PRIu64 "d)", node.TotalCallCount, node.DrawCallCount);
		}
	}
	else
	{
		ImGui::Text("%" PRIu64, node.TotalCallCount);
	}

	if (open)
	{
		std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*> childPtrs;
		childPtrs.reserve(node.Children.size());
		for (const Engine::Diagnostics::ProfilerSnapshotNode& child : node.Children)
		{
			childPtrs.push_back(&child);
		}
		RenderTableRows(childPtrs, depth + 1);
		ImGui::TreePop();
	}
}

void ProfilerPanel::BeginProfilerTable(const char* id) const
{
	ImGui::PushID(id);
	if (ImGui::BeginTable("##Table", 5, kTableFlags))
	{
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Incl", ImGuiTableColumnFlags_WidthFixed, 56.0f);
		ImGui::TableSetupColumn("Excl", ImGuiTableColumnFlags_WidthFixed, 56.0f);
		ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 56.0f);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 48.0f);
		ImGui::TableHeadersRow();
	}
}

void ProfilerPanel::SortBucket(std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& bucket) const
{
	if (m_sortMode == SortMode::Hierarchy)
	{
		return;
	}
	const SortMode mode = m_sortMode;
	std::stable_sort(
	    bucket.begin(),
	    bucket.end(),
	    [mode](const Engine::Diagnostics::ProfilerSnapshotNode* lhs,
	           const Engine::Diagnostics::ProfilerSnapshotNode* rhs)
	    {
		    auto exclusiveOf = [](const Engine::Diagnostics::ProfilerSnapshotNode* node)
		    {
			    return std::max(0.0, node->AverageDurationMicroseconds - SumChildAverages(node->Children));
		    };
		    switch (mode)
		    {
			    case SortMode::AlphabeticalAsc:
				    return lhs->Name < rhs->Name;
			    case SortMode::AlphabeticalDesc:
				    return lhs->Name > rhs->Name;
			    case SortMode::InclusiveDescending:
				    return lhs->AverageDurationMicroseconds > rhs->AverageDurationMicroseconds;
			    case SortMode::ExclusiveDescending:
				    return exclusiveOf(lhs) > exclusiveOf(rhs);
			    case SortMode::MaxDescending:
				    return lhs->MaxDurationMicroseconds > rhs->MaxDurationMicroseconds;
			    case SortMode::CallsDescending:
				    return lhs->TotalCallCount > rhs->TotalCallCount;
			    case SortMode::Hierarchy:
			    default:
				    return false;
		    }
	    });
}

void ProfilerPanel::RenderTableRows(
    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& nodes,
    int depth) const
{
	for (const Engine::Diagnostics::ProfilerSnapshotNode* node : nodes)
	{
		RenderNodeRow(*node, depth);
	}
}

void ProfilerPanel::RenderGroupedNodes(const std::vector<Engine::Diagnostics::ProfilerSnapshotNode>& nodes) const
{
	if (nodes.empty())
	{
		return;
	}

	// Build module groups.
	std::vector<std::string_view> moduleOrder;
	std::unordered_map<std::string_view, std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>> grouped;
	for (const Engine::Diagnostics::ProfilerSnapshotNode& node : nodes)
	{
		const std::string_view moduleName = ExtractModuleName(node.Name);
		auto [it, inserted] = grouped.try_emplace(moduleName);
		if (inserted)
		{
			moduleOrder.push_back(moduleName);
		}
		it->second.push_back(&node);
	}

	// Single module — no grouping headers, just one table.
	if (moduleOrder.size() < 2)
	{
		std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*> bucket;
		bucket.reserve(nodes.size());
		for (const Engine::Diagnostics::ProfilerSnapshotNode& node : nodes)
		{
			bucket.push_back(&node);
		}
		SortBucket(bucket);
		BeginProfilerTable("ungrouped");
		RenderTableRows(bucket, 0);
		ImGui::EndTable();
		ImGui::PopID();

		if (m_showCharts && bucket.size() >= 2)
		{
			RenderModuleCharts(bucket, moduleOrder[0]);
		}
		return;
	}

	// Multiple modules — each gets a collapsing header + its own table + chart.
	for (std::string_view moduleName : moduleOrder)
	{
		auto& bucket = grouped[moduleName];
		SortBucket(bucket);

		std::string headerLabel{"["};
		headerLabel.append(moduleName.begin(), moduleName.end());
		headerLabel += "]";

		const ImGuiTreeNodeFlags groupFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;
		ImGui::PushID(headerLabel.c_str());
		const bool groupOpen = ImGui::CollapsingHeader(headerLabel.c_str(), groupFlags);
		if (groupOpen)
		{
			BeginProfilerTable(headerLabel.c_str());
			RenderTableRows(bucket, 0);
			ImGui::EndTable();
			ImGui::PopID(); // from BeginProfilerTable

			if (m_showCharts && bucket.size() >= 2)
			{
				RenderModuleCharts(bucket, moduleName);
			}
		}
		ImGui::PopID();
	}
}

void ProfilerPanel::RenderModuleCharts(
    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& bucket,
    std::string_view moduleName) const
{
	struct Slice
	{
		std::string ShortName;
		double ValueMs = 0.0;
		ImU32 Color = 0;
	};

	auto hashColor = [](std::string_view name, float alpha = 0.90f) noexcept -> ImU32
	{
		std::uint32_t h = 2166136261u;
		for (char c : name)
		{
			h ^= static_cast<std::uint32_t>(static_cast<unsigned char>(c));
			h *= 16777619u;
		}
		const float hue = static_cast<float>(h % 360u) / 360.0f;
		ImVec4 rgb;
		ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.85f, rgb.x, rgb.y, rgb.z);
		rgb.w = alpha;
		return ImGui::ColorConvertFloat4ToU32(rgb);
	};

	auto shortenName = [](const std::string& fullName) -> std::string
	{
		// Take last dotted segment, or after "] " for GPU labels.
		if (const std::size_t bracket = fullName.find("] "); bracket != std::string::npos)
		{
			return fullName.substr(bracket + 2);
		}
		if (const std::size_t dot = fullName.rfind('.'); dot != std::string::npos)
		{
			return fullName.substr(dot + 1);
		}
		return fullName;
	};

	std::vector<Slice> slices;
	slices.reserve(bucket.size());
	double totalMs = 0.0;
	double maxMs = 0.0;
	for (const Engine::Diagnostics::ProfilerSnapshotNode* node : bucket)
	{
		const double inclusiveMs = node->AverageDurationMicroseconds * kMicrosecondsToMilliseconds;
		const double childSumMs = SumChildAverages(node->Children) * kMicrosecondsToMilliseconds;
		const double exclusiveMs = std::max(0.0, inclusiveMs - childSumMs);
		slices.push_back(Slice{shortenName(node->Name), exclusiveMs, hashColor(node->Name)});
		totalMs += exclusiveMs;
		maxMs = std::max(maxMs, exclusiveMs);
	}
	if (totalMs <= 0.0)
	{
		return;
	}

	constexpr float kPi = 3.14159265358979323846f;
	const float availWidth = ImGui::GetContentRegionAvail().x;
	if (availWidth < 200.0f)
	{
		return;
	}

	const float rowHeight = ImGui::GetFontSize() + 4.0f;
	const float pad = 8.0f;
	const float pieSize = 100.0f;
	const float pieRadius = pieSize * 0.5f;
	const float donutHole = pieRadius * 0.45f;

	// Left column: pie + legend stacked vertically.
	const float legendHeight = rowHeight * static_cast<float>(slices.size());
	const float leftColumnWidth = std::max(pieSize + 16.0f, 160.0f);
	const float leftColumnHeight = pieSize + 8.0f + legendHeight;

	// Right column: vertical bar chart.
	const float barChartHeight = pieSize;   // Match pie height for visual alignment.
	const float barLabelHeight = rowHeight;
	const float rightColumnHeight = barChartHeight + barLabelHeight + 4.0f;

	const float chartHeight = std::max(leftColumnHeight, rightColumnHeight) + rowHeight + 12.0f;

	ImGui::PushID(moduleName.data() != nullptr ? moduleName.data() : "charts");
	ImGui::BeginChild("##Charts", ImVec2(availWidth, chartHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();

	// ====== LEFT: Donut pie ======
	const ImVec2 pieCenter{origin.x + pad + pieRadius, origin.y + pieRadius + 4.0f};

	double cursor = 0.0;
	for (const Slice& slice : slices)
	{
		const double startFrac = cursor / totalMs;
		cursor += slice.ValueMs;
		const double endFrac = cursor / totalMs;
		if (endFrac <= startFrac)
		{
			continue;
		}
		const float startAngle = static_cast<float>(startFrac * 2.0 * kPi) - kPi * 0.5f;
		const float endAngle = static_cast<float>(endFrac * 2.0 * kPi) - kPi * 0.5f;
		drawList->PathLineTo(pieCenter);
		drawList->PathArcTo(pieCenter, pieRadius, startAngle, endAngle, 32);
		drawList->PathFillConvex(slice.Color);

		// Hover detection on pie slice.
		const ImVec2 mousePos = ImGui::GetMousePos();
		const ImVec2 toMouse{mousePos.x - pieCenter.x, mousePos.y - pieCenter.y};
		const float distSq = toMouse.x * toMouse.x + toMouse.y * toMouse.y;
		if (distSq <= pieRadius * pieRadius && distSq >= donutHole * donutHole)
		{
			float angle = std::atan2(toMouse.y, toMouse.x) + kPi * 0.5f;
			if (angle < 0.0f)
			{
				angle += 2.0f * kPi;
			}
			const float sStart = startAngle + kPi * 0.5f;
			const float sEnd = endAngle + kPi * 0.5f;
			if (angle >= sStart && angle <= sEnd)
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s: %.3f ms (%.1f%%)", slice.ShortName.c_str(), slice.ValueMs, slice.ValueMs / totalMs * 100.0);
				ImGui::EndTooltip();
			}
		}
	}
	drawList->AddCircleFilled(pieCenter, donutHole, IM_COL32(22, 22, 26, 255), 32);
	char centerText[16];
	std::snprintf(centerText, sizeof(centerText), "%.2f", totalMs);
	const ImVec2 centerSize = ImGui::CalcTextSize(centerText);
	drawList->AddText(
	    ImVec2(pieCenter.x - centerSize.x * 0.5f, pieCenter.y - centerSize.y * 0.5f),
	    IM_COL32(200, 200, 200, 220),
	    centerText);

	// Legend below pie.
	const float legendX = origin.x + pad;
	float legendY = pieCenter.y + pieRadius + 8.0f;
	for (const Slice& slice : slices)
	{
		drawList->AddRectFilled(
		    ImVec2(legendX, legendY + 2.0f),
		    ImVec2(legendX + 10.0f, legendY + rowHeight - 2.0f),
		    slice.Color,
		    2.0f);
		char legendStr[128];
		std::snprintf(legendStr, sizeof(legendStr), "%s  %.3f ms", slice.ShortName.c_str(), slice.ValueMs);
		drawList->AddText(ImVec2(legendX + 14.0f, legendY), IM_COL32(210, 210, 210, 230), legendStr);
		legendY += rowHeight;
	}

	// ====== RIGHT: Vertical column bar chart ======
	const float barAreaLeft = origin.x + leftColumnWidth + pad;
	const float barAreaTop = origin.y + 4.0f;
	const float barAreaWidth = availWidth - leftColumnWidth - pad * 2.0f;
	if (barAreaWidth > 40.0f)
	{
		const float barSpacing = 3.0f;
		const float numSlices = static_cast<float>(slices.size());
		const float barSlotWidth = (barAreaWidth - barSpacing * (numSlices - 1.0f)) / numSlices;

		// Background.
		drawList->AddRectFilled(
		    ImVec2(barAreaLeft, barAreaTop),
		    ImVec2(barAreaLeft + barAreaWidth, barAreaTop + barChartHeight),
		    IM_COL32(25, 25, 30, 180),
		    3.0f);

		for (std::size_t i = 0; i < slices.size(); ++i)
		{
			const Slice& slice = slices[i];
			const float xLeft = barAreaLeft + static_cast<float>(i) * (barSlotWidth + barSpacing);
			const float xRight = xLeft + barSlotWidth;
			const float fraction = maxMs > 0.0 ? static_cast<float>(slice.ValueMs / maxMs) : 0.0f;
			const float barTop = barAreaTop + barChartHeight * (1.0f - fraction);
			const float barBottom = barAreaTop + barChartHeight;
			drawList->AddRectFilled(ImVec2(xLeft, barTop), ImVec2(xRight, barBottom), slice.Color, 2.0f);

			// Value label above bar.
			char valueText[16];
			std::snprintf(valueText, sizeof(valueText), "%.2f", slice.ValueMs);
			const ImVec2 textSize = ImGui::CalcTextSize(valueText);
			if (barSlotWidth >= textSize.x + 2.0f)
			{
				const float textX = xLeft + (barSlotWidth - textSize.x) * 0.5f;
				const float textY = std::max(barAreaTop, barTop - textSize.y - 1.0f);
				drawList->AddText(ImVec2(textX, textY), IM_COL32(220, 220, 220, 220), valueText);
			}

			// Name label below bar.
			const ImVec2 nameSize = ImGui::CalcTextSize(slice.ShortName.c_str());
			if (barSlotWidth >= nameSize.x + 2.0f)
			{
				drawList->AddText(
				    ImVec2(xLeft + (barSlotWidth - nameSize.x) * 0.5f, barBottom + 2.0f),
				    IM_COL32(180, 180, 180, 200),
				    slice.ShortName.c_str());
			}

			// Hover tooltip.
			if (ImGui::IsMouseHoveringRect(ImVec2(xLeft, barAreaTop), ImVec2(xRight, barBottom + barLabelHeight)))
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s: %.3f ms (%.1f%%)", slice.ShortName.c_str(), slice.ValueMs, slice.ValueMs / totalMs * 100.0);
				ImGui::EndTooltip();
			}
		}
	}

	// Footer.
	char totalText[64];
	std::snprintf(totalText, sizeof(totalText), "Excl total: %.3f ms", totalMs);
	drawList->AddText(
	    ImVec2(origin.x + pad, origin.y + chartHeight - rowHeight),
	    IM_COL32(160, 160, 160, 200),
	    totalText);

	ImGui::Dummy(ImVec2(availWidth, chartHeight));
	ImGui::EndChild();
	ImGui::PopID();
}
