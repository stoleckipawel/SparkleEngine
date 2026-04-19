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
		double Pct = 0.0;
		ImU32 Color = 0;
	};

	auto hashColor = [](std::string_view name) noexcept -> ImU32
	{
		std::uint32_t h = 2166136261u;
		for (char c : name)
		{
			h ^= static_cast<std::uint32_t>(static_cast<unsigned char>(c));
			h *= 16777619u;
		}
		const float hue = static_cast<float>(h % 360u) / 360.0f;
		ImVec4 rgb;
		ImGui::ColorConvertHSVtoRGB(hue, 0.50f, 0.78f, rgb.x, rgb.y, rgb.z);
		rgb.w = 1.0f;
		return ImGui::ColorConvertFloat4ToU32(rgb);
	};

	auto shortenName = [](const std::string& fullName) -> std::string
	{
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

	// ---- Build data ----
	std::vector<Slice> slices;
	slices.reserve(bucket.size());
	double totalMs = 0.0;
	double maxMs = 0.0;
	for (const Engine::Diagnostics::ProfilerSnapshotNode* node : bucket)
	{
		const double ms = node->AverageDurationMicroseconds * kMicrosecondsToMilliseconds;
		slices.push_back(Slice{shortenName(node->Name), ms, 0.0, hashColor(node->Name)});
		totalMs += ms;
		maxMs = std::max(maxMs, ms);
	}
	if (totalMs <= 0.0)
	{
		return;
	}
	for (Slice& s : slices)
	{
		s.Pct = s.ValueMs / totalMs * 100.0;
	}

	const float availWidth = ImGui::GetContentRegionAvail().x;
	if (availWidth < 220.0f)
	{
		return;
	}

	// ---- Layout constants ----
	constexpr float kPi = 3.14159265358979323846f;
	const float fontSize = ImGui::GetFontSize();
	const float rowH = fontSize + 3.0f;
	const float outerPad = 6.0f;
	const float innerPad = 10.0f;
	const float innerWidth = availWidth - outerPad * 2.0f;

	// Pie dimensions.
	const float pieSize = 80.0f;
	const float pieRadius = pieSize * 0.5f;
	const float donutHole = pieRadius * 0.40f;

	// Legend occupies space to the right of pie within the left panel.
	const float legendRowH = fontSize * 2.0f + 5.0f;
	const float legendHeight = legendRowH * static_cast<float>(slices.size());
	const float pieLegendHeight = std::max(pieSize, legendHeight);

	// Right panel: bar chart.
	const float barChartH = pieLegendHeight;
	const float barLabelH = fontSize + 2.0f;

	// Total card height.
	const float cardContentH = pieLegendHeight + barLabelH + innerPad + rowH;
	const float cardH = cardContentH + outerPad * 2.0f;

	// ---- Card container ----
	ImGui::Spacing();
	ImGui::PushID(moduleName.data() != nullptr ? moduleName.data() : "charts");
	ImGui::BeginChild("##Charts", ImVec2(availWidth, cardH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 cardOrigin = ImGui::GetCursorScreenPos();

	// Card background.
	dl->AddRectFilled(
	    cardOrigin,
	    ImVec2(cardOrigin.x + availWidth, cardOrigin.y + cardH),
	    IM_COL32(28, 28, 34, 230),
	    4.0f);
	dl->AddRect(
	    cardOrigin,
	    ImVec2(cardOrigin.x + availWidth, cardOrigin.y + cardH),
	    IM_COL32(55, 55, 65, 160),
	    4.0f);

	const float cx = cardOrigin.x + outerPad;
	float cy = cardOrigin.y + outerPad;

	// ============ Donut pie (left) | Legend (middle) | Bar chart (right) ============
	// Split: pie gets fixed space, legend gets fixed space, bar chart gets the rest.
	const float pieBlockW = pieSize + innerPad;
	const float legendColW = 170.0f;
	const float dividerGap = innerPad;
	const float barAreaLeft = cx + pieBlockW + legendColW + dividerGap;
	const float barAreaW = (cx + innerWidth) - barAreaLeft;

	// ---- Donut pie ----
	const ImVec2 pieCenter{cx + pieRadius, cy + pieLegendHeight * 0.5f};

	double angleCursor = 0.0;
	for (const Slice& s : slices)
	{
		const double startFrac = angleCursor / totalMs;
		angleCursor += s.ValueMs;
		const double endFrac = angleCursor / totalMs;
		if (endFrac <= startFrac)
		{
			continue;
		}
		const float a0 = static_cast<float>(startFrac * 2.0 * kPi) - kPi * 0.5f;
		const float a1 = static_cast<float>(endFrac * 2.0 * kPi) - kPi * 0.5f;
		dl->PathLineTo(pieCenter);
		dl->PathArcTo(pieCenter, pieRadius, a0, a1, 32);
		dl->PathFillConvex(s.Color);

		// Hover.
		const ImVec2 mp = ImGui::GetMousePos();
		const ImVec2 tm{mp.x - pieCenter.x, mp.y - pieCenter.y};
		const float dSq = tm.x * tm.x + tm.y * tm.y;
		if (dSq <= pieRadius * pieRadius && dSq >= donutHole * donutHole)
		{
			float ang = std::atan2(tm.y, tm.x) + kPi * 0.5f;
			if (ang < 0.0f)
			{
				ang += 2.0f * kPi;
			}
			if (ang >= (a0 + kPi * 0.5f) && ang <= (a1 + kPi * 0.5f))
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s: %.3f ms (%.1f%%)", s.ShortName.c_str(), s.ValueMs, s.Pct);
				ImGui::EndTooltip();
			}
		}
	}
	// Hole.
	dl->AddCircleFilled(pieCenter, donutHole, IM_COL32(28, 28, 34, 255), 32);
	char centerBuf[16];
	std::snprintf(centerBuf, sizeof(centerBuf), "%.2f", totalMs);
	const ImVec2 cSz = ImGui::CalcTextSize(centerBuf);
	dl->AddText(ImVec2(pieCenter.x - cSz.x * 0.5f, pieCenter.y - cSz.y * 0.5f), IM_COL32(200, 200, 200, 220), centerBuf);

	// Label under pie.
	{
		const char* unitLabel = "ms (excl)";
		const ImVec2 ulSz = ImGui::CalcTextSize(unitLabel);
		dl->AddText(
		    ImVec2(pieCenter.x - ulSz.x * 0.5f, pieCenter.y + pieRadius + 2.0f),
		    IM_COL32(120, 120, 130, 180),
		    unitLabel);
	}

	// ---- Legend (right of pie) ----
	{
		const float lx = cx + pieBlockW;
		float ly = cy + (pieLegendHeight - legendHeight) * 0.5f;
		for (const Slice& s : slices)
		{
			dl->AddRectFilled(ImVec2(lx, ly + 3.0f), ImVec2(lx + 8.0f, ly + fontSize - 1.0f), s.Color, 2.0f);
			dl->AddText(ImVec2(lx + 12.0f, ly), IM_COL32(200, 200, 200, 220), s.ShortName.c_str());

			// Value on second line, indented.
			char valBuf[32];
			std::snprintf(valBuf, sizeof(valBuf), "%.3f ms  (%.0f%%)", s.ValueMs, s.Pct);
			dl->AddText(ImVec2(lx + 12.0f, ly + fontSize + 1.0f), IM_COL32(140, 140, 150, 190), valBuf);
			ly += legendRowH;
		}
	}

	// ---- Vertical divider ----
	{
		const float divX = barAreaLeft - dividerGap * 0.5f;
		dl->AddLine(ImVec2(divX, cy + 2.0f), ImVec2(divX, cy + pieLegendHeight - 2.0f), IM_COL32(60, 60, 70, 120));
	}

	// ---- Column bar chart (right) ----
	if (barAreaW > 50.0f)
	{
		const float barSpacing = 4.0f;
		const float numSlices = static_cast<float>(slices.size());
		const float slotW = (barAreaW - barSpacing * (numSlices - 1.0f)) / numSlices;
		const float maxBarH = barChartH - 4.0f;

		// Subtle grid lines (3 horizontal).
		for (int g = 1; g <= 3; ++g)
		{
			const float gy = cy + barChartH - maxBarH * (static_cast<float>(g) / 4.0f);
			dl->AddLine(ImVec2(barAreaLeft, gy), ImVec2(barAreaLeft + barAreaW, gy), IM_COL32(50, 50, 58, 80));
		}

		for (std::size_t i = 0; i < slices.size(); ++i)
		{
			const Slice& s = slices[i];
			const float xL = barAreaLeft + static_cast<float>(i) * (slotW + barSpacing);
			const float xR = xL + slotW;
			const float frac = maxMs > 0.0 ? static_cast<float>(s.ValueMs / maxMs) : 0.0f;
			const float barH = maxBarH * frac;
			const float bTop = cy + barChartH - barH;
			const float bBot = cy + barChartH;

			// Bar with subtle gradient — slightly lighter at top.
			const ImU32 colTop = s.Color;
			ImVec4 colBotRgb = ImGui::ColorConvertU32ToFloat4(s.Color);
			colBotRgb.x *= 0.7f;
			colBotRgb.y *= 0.7f;
			colBotRgb.z *= 0.7f;
			const ImU32 colBot = ImGui::ColorConvertFloat4ToU32(colBotRgb);
			dl->AddRectFilledMultiColor(ImVec2(xL, bTop), ImVec2(xR, bBot), colTop, colTop, colBot, colBot);

			// Value above bar.
			char vBuf[16];
			std::snprintf(vBuf, sizeof(vBuf), "%.2f", s.ValueMs);
			const ImVec2 vSz = ImGui::CalcTextSize(vBuf);
			if (slotW >= vSz.x + 2.0f)
			{
				dl->AddText(
				    ImVec2(xL + (slotW - vSz.x) * 0.5f, std::max(cy, bTop - vSz.y - 1.0f)),
				    IM_COL32(210, 210, 210, 210),
				    vBuf);
			}

			// Name below bar.
			const ImVec2 nSz = ImGui::CalcTextSize(s.ShortName.c_str());
			if (slotW >= nSz.x + 2.0f)
			{
				dl->AddText(
				    ImVec2(xL + (slotW - nSz.x) * 0.5f, bBot + 2.0f),
				    IM_COL32(160, 160, 170, 190),
				    s.ShortName.c_str());
			}

			// Hover.
			if (ImGui::IsMouseHoveringRect(ImVec2(xL, cy), ImVec2(xR, bBot + barLabelH)))
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s: %.3f ms (%.1f%%)", s.ShortName.c_str(), s.ValueMs, s.Pct);
				ImGui::EndTooltip();
			}
		}
	}

	// ============ Footer ============
	{
		char totalBuf[64];
		std::snprintf(totalBuf, sizeof(totalBuf), "Inclusive total: %.3f ms", totalMs);
		const float footY = cardOrigin.y + cardH - outerPad - rowH + 2.0f;
		dl->AddText(ImVec2(cx, footY), IM_COL32(130, 130, 140, 180), totalBuf);
	}

	ImGui::Dummy(ImVec2(availWidth, cardH));
	ImGui::EndChild();
	ImGui::PopID();
}
