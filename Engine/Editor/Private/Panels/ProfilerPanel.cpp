#include "PCH.h"
#include "Panels/ProfilerPanel.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <numeric>
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

	std::string_view ShortenScopeName(std::string_view fullName) noexcept
	{
		if (const std::size_t bracket = fullName.find("] "); bracket != std::string_view::npos)
		{
			return fullName.substr(bracket + 2);
		}
		if (const std::size_t dot = fullName.rfind('.'); dot != std::string_view::npos)
		{
			return fullName.substr(dot + 1);
		}
		return fullName;
	}

	ImU32 GenerateProfilerColor(std::string_view /*shortName*/, std::size_t index, std::size_t /*total*/, float saturation = 1.0f) noexcept
	{
		// Curated palette inspired by Tableau 10 / Chrome DevTools performance —
		// muted but distinguishable, reads cleanly on a dark background.
		static constexpr ImU32 kPalette[] = {
		    IM_COL32(0x4E, 0x79, 0xA7, 255), // blue
		    IM_COL32(0xF2, 0x8E, 0x2B, 255), // orange
		    IM_COL32(0x59, 0xA1, 0x4F, 255), // green
		    IM_COL32(0xE1, 0x57, 0x59, 255), // red
		    IM_COL32(0xB0, 0x7A, 0xA1, 255), // purple
		    IM_COL32(0xED, 0xC9, 0x49, 255), // yellow
		    IM_COL32(0x76, 0xB7, 0xB2, 255), // teal
		    IM_COL32(0xFF, 0x9D, 0xA7, 255), // pink
		    IM_COL32(0x9C, 0x75, 0x5F, 255), // brown
		    IM_COL32(0xBA, 0xB0, 0xAC, 255), // gray
		};
		const ImU32 base = kPalette[index % (sizeof(kPalette) / sizeof(kPalette[0]))];
		if (saturation >= 0.999f)
		{
			return base;
		}
		// Optionally desaturate toward neutral gray for table tints.
		ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(base);
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(rgb.x, rgb.y, rgb.z, h, s, v);
		s *= saturation;
		ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
		rgb.w = 1.0f;
		return ImGui::ColorConvertFloat4ToU32(rgb);
	}

	void RightAlignedText(const char* fmt, ...)
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
	RenderToolbar();

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

void ProfilerPanel::RenderToolbar() noexcept
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

	// Toolbar row: "Sort:" label + dropdown on the left, frame summary on the right.
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("SORT");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(170.0f);
	int sortIndex = static_cast<int>(m_sortMode);
	if (ImGui::Combo("##SortCombo", &sortIndex, kSortLabels, IM_ARRAYSIZE(kSortLabels)))
	{
		m_sortMode = static_cast<SortMode>(sortIndex);
	}

	// Right-aligned summary: scope counts and total sample frames.
	std::size_t cpuScopeCount = 0;
	for (const Engine::Diagnostics::ProfilerThreadSnapshot& thread : m_snapshot.CpuThreads)
	{
		cpuScopeCount += thread.Roots.size();
	}
	const std::size_t gpuPassCount = !m_snapshot.GpuRoots.empty() && m_snapshot.GpuRoots.size() == 1
	    ? m_snapshot.GpuRoots[0].Children.size()
	    : m_snapshot.GpuRoots.size();

	char summaryBuf[96];
	std::snprintf(summaryBuf, sizeof(summaryBuf),
	    "CPU threads: %zu  \xC2\xB7  GPU passes: %zu",
	    m_snapshot.CpuThreads.size(), gpuPassCount);
	(void)cpuScopeCount;
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

	// Show GPU roots as-is; "GPU Frame" is the top-level scope.
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

	// Charts use the deepest single-child drill-in to reach the actual passes.
	const std::vector<Engine::Diagnostics::ProfilerSnapshotNode>* chartLevel = &m_snapshot.GpuRoots;
	while (chartLevel->size() == 1 && !(*chartLevel)[0].Children.empty())
	{
		chartLevel = &(*chartLevel)[0].Children;
	}
	if (chartLevel->size() >= 2)
	{
		std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*> chartBucket;
		chartBucket.reserve(chartLevel->size());
		for (const Engine::Diagnostics::ProfilerSnapshotNode& node : *chartLevel)
		{
			chartBucket.push_back(&node);
		}
		RenderModuleCharts(chartBucket, "GPU");
	}
}

void ProfilerPanel::RenderNodeRow(const Engine::Diagnostics::ProfilerSnapshotNode& node, int depth, std::size_t siblingIndex, std::size_t siblingTotal) const
{
	ImGui::TableNextRow();

	// Tint rows with a low-opacity color matching the charts.
	if (siblingTotal > 0)
	{
		const std::string_view shortName = ShortenScopeName(node.Name);
		ImU32 color = GenerateProfilerColor(shortName, siblingIndex, siblingTotal, 0.85f);
		// Override alpha to ~25/255 for a subtle tint.
		color = (color & 0x00FFFFFFu) | (0x19u << 24);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, color);
	}

	// Visibility toggle column — only for depth-0 (chart-visible) scopes.
	ImGui::TableSetColumnIndex(0);
	if (depth == 0)
	{
		const bool isHidden = m_hiddenScopes.count(node.Name) > 0;
		const ImU32 dotColor = (siblingTotal > 0)
		    ? GenerateProfilerColor(ShortenScopeName(node.Name), siblingIndex, siblingTotal)
		    : IM_COL32(160, 165, 180, 200);
		ImGui::PushID(node.Name.c_str());

		// Draw a small clickable colored circle (filled if visible, hollow ring if hidden).
		const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		const float radius = ImGui::GetFontSize() * 0.3f;
		const ImVec2 center{cursorPos.x + radius + 2.0f, cursorPos.y + ImGui::GetTextLineHeight() * 0.5f};
		ImDrawList* rowDl = ImGui::GetWindowDrawList();
		if (isHidden)
		{
			rowDl->AddCircle(center, radius, (dotColor & 0x00FFFFFFu) | (0x60u << 24), 12, 1.5f);
		}
		else
		{
			rowDl->AddCircleFilled(center, radius, dotColor, 12);
		}
		// Invisible button over the dot for click detection.
		if (ImGui::InvisibleButton("##vis", ImVec2(radius * 2.0f + 4.0f, ImGui::GetTextLineHeight())))
		{
			if (isHidden)
			{
				m_hiddenScopes.erase(node.Name);
			}
			else
			{
				m_hiddenScopes.insert(node.Name);
			}
		}
		ImGui::PopID();
	}

	ImGui::TableSetColumnIndex(1);
	const bool hasChildren = !node.Children.empty();
	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth
	                               | (hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
	if (depth == 0)
	{
		nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	const std::string_view displayName = ShortenScopeName(node.Name);
	const std::string displayNameStr(displayName);
	const bool open = ImGui::TreeNodeEx(displayNameStr.c_str(), nodeFlags);

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

	ImGui::TableSetColumnIndex(2);
	RightAlignedText("%.3f", inclusiveMs);
	ImGui::TableSetColumnIndex(3);
	RightAlignedText("%.3f", exclusiveMs);
	ImGui::TableSetColumnIndex(4);
	RightAlignedText("%.3f", node.MaxDurationMicroseconds * kMicrosecondsToMilliseconds);
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
	if (ImGui::BeginTable("##Table", 6, kTableFlags))
	{
		ImGui::TableSetupColumn("##Vis", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20.0f);
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Incl ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("Excl ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("Max ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 64.0f);
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
    int depth,
    std::size_t siblingOffset,
    std::size_t siblingTotal) const
{
	const std::size_t total = siblingTotal > 0 ? siblingTotal : nodes.size();
	for (std::size_t i = 0; i < nodes.size(); ++i)
	{
		RenderNodeRow(*nodes[i], depth, siblingOffset + i, total);
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

		if (bucket.size() >= 2)
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

			if (bucket.size() >= 2)
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

	auto hashColor = [](std::string_view name, std::size_t index, std::size_t total) noexcept -> ImU32
	{
		return GenerateProfilerColor(name, index, total);
	};

	auto shortenName = [](const std::string& fullName) -> std::string
	{
		const std::string_view sv = ShortenScopeName(fullName);
		return std::string(sv);
	};

	// ---- Build data ----
	std::vector<Slice> slices;
	slices.reserve(bucket.size());
	double totalMs = 0.0;
	double maxMs = 0.0;
	for (std::size_t i = 0; i < bucket.size(); ++i)
	{
		const Engine::Diagnostics::ProfilerSnapshotNode* node = bucket[i];
		if (m_hiddenScopes.count(node->Name) > 0)
		{
			continue;
		}
		const double ms = node->AverageDurationMicroseconds * kMicrosecondsToMilliseconds;
		std::string shortName = shortenName(node->Name);
		const ImU32 color = hashColor(shortName, i, bucket.size());
		slices.push_back(Slice{std::move(shortName), ms, 0.0, color});
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
	// Sorted indices for the bar chart (most expensive → cheapest). Pie keeps insertion order.
	std::vector<std::size_t> barOrder(slices.size());
	std::iota(barOrder.begin(), barOrder.end(), std::size_t{0});
	std::sort(barOrder.begin(), barOrder.end(), [&](std::size_t a, std::size_t b) { return slices[a].ValueMs > slices[b].ValueMs; });

	const float availWidth = ImGui::GetContentRegionAvail().x;
	if (availWidth < 260.0f)
	{
		return;
	}

	// ---- Palette ----
	constexpr ImU32 kColCardBg = IM_COL32(24, 25, 30, 230);
	constexpr ImU32 kColCardBorder = IM_COL32(48, 50, 58, 140);
	constexpr ImU32 kColDivider = IM_COL32(48, 50, 58, 140);
	constexpr ImU32 kColAxis = IM_COL32(70, 72, 82, 200);
	constexpr ImU32 kColGrid = IM_COL32(50, 52, 60, 90);
	constexpr ImU32 kColTextStrong = IM_COL32(220, 222, 228, 230);
	constexpr ImU32 kColTextMuted = IM_COL32(140, 144, 156, 200);
	constexpr ImU32 kColTextDim = IM_COL32(105, 110, 122, 200);
	constexpr ImU32 kColTitle = IM_COL32(170, 175, 190, 220);

	// ---- Layout constants ----
	constexpr float kPi = 3.14159265358979323846f;
	const float fontSize = ImGui::GetFontSize();
	const float rowH = fontSize + 3.0f;
	const float outerPad = 10.0f;
	const float innerPad = 14.0f;
	const float innerWidth = availWidth - outerPad * 2.0f;

	// Section header band.
	const float titleBarH = rowH + 2.0f;

	// Pie panel — narrower, cleaner proportions.
	const float piePanelW = std::min(innerWidth * 0.33f, 200.0f);
	const float pieMaxSize = std::min(piePanelW - 24.0f, 130.0f);
	const float pieRadius = pieMaxSize * 0.5f;
	const float donutHole = pieRadius * 0.55f; // thinner ring for refinement
	const float pieCaptionH = fontSize + 4.0f;

	// Bar chart geometry.
	const float yAxisLabelW = ImGui::CalcTextSize("88.8").x + 6.0f; // "x.xx" width
	const float barValLabelH = fontSize + 2.0f;

	// X-axis labels are always rotated for consistent, scannable layout.
	float maxLabelW = 0.0f;
	for (const Slice& s : slices)
	{
		maxLabelW = std::max(maxLabelW, ImGui::CalcTextSize(s.ShortName.c_str()).x);
	}
	constexpr float kLabelAngleDeg = 35.0f;
	const float labelAngleRad = kLabelAngleDeg * (3.14159265f / 180.0f);
	const float barXLabelH = maxLabelW * std::sin(labelAngleRad) + fontSize * std::cos(labelAngleRad) + 6.0f;

	// Chart content height: pick something taller than wide for nicer bar proportions.
	const float chartContentH = std::max(pieMaxSize + pieCaptionH, 150.0f);

	// Footer band.
	const float footerH = rowH + 2.0f;

	// Total card height.
	const float cardContentH = titleBarH + innerPad + chartContentH + barXLabelH + innerPad + footerH;
	const float cardH = cardContentH + outerPad * 2.0f;

	// ---- Card container ----
	ImGui::Spacing();
	ImGui::PushID(moduleName.data() != nullptr ? moduleName.data() : "charts");
	ImGui::BeginChild("##Charts", ImVec2(availWidth, cardH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 cardOrigin = ImGui::GetCursorScreenPos();

	// Card background.
	dl->AddRectFilled(cardOrigin, ImVec2(cardOrigin.x + availWidth, cardOrigin.y + cardH), kColCardBg, 4.0f);
	dl->AddRect(cardOrigin, ImVec2(cardOrigin.x + availWidth, cardOrigin.y + cardH), kColCardBorder, 4.0f);

	const float cx = cardOrigin.x + outerPad;
	float cy = cardOrigin.y + outerPad;

	// ============ Header band ============
	const float pieAreaLeft = cx;
	const float pieAreaRight = cx + piePanelW;
	const float dividerX = pieAreaRight + innerPad * 0.5f;
	const float barAreaLeft = pieAreaRight + innerPad;
	const float barAreaRight = cx + innerWidth;

	// Section titles.
	{
		dl->AddText(ImVec2(pieAreaLeft, cy), kColTitle, "Distribution");
		const char* rightTitle = "Pass timings (ms)";
		dl->AddText(ImVec2(barAreaLeft, cy), kColTitle, rightTitle);
	}
	cy += titleBarH + innerPad;

	const float chartTop = cy;
	const float chartBot = cy + chartContentH;

	// ---- Vertical divider ----
	dl->AddLine(ImVec2(dividerX, chartTop - 4.0f), ImVec2(dividerX, chartBot + barXLabelH + 2.0f), kColDivider);

	// ============ Donut pie (left) ============
	const ImVec2 pieCenter{pieAreaLeft + piePanelW * 0.5f, chartTop + (chartContentH - pieCaptionH) * 0.5f};

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
		dl->PathArcTo(pieCenter, pieRadius, a0, a1, 48);
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
	// Hole + thin separator ring.
	dl->AddCircleFilled(pieCenter, donutHole, kColCardBg, 48);
	dl->AddCircle(pieCenter, donutHole, kColCardBorder, 48, 1.0f);
	dl->AddCircle(pieCenter, pieRadius, kColCardBorder, 48, 1.0f);

	// Center text — large value + "ms" suffix.
	{
		char centerBuf[16];
		std::snprintf(centerBuf, sizeof(centerBuf), "%.2f", totalMs);
		const ImVec2 cSz = ImGui::CalcTextSize(centerBuf);
		const float cTextY = pieCenter.y - cSz.y;
		dl->AddText(ImVec2(pieCenter.x - cSz.x * 0.5f, cTextY), kColTextStrong, centerBuf);
		const ImVec2 msSz = ImGui::CalcTextSize("ms");
		dl->AddText(ImVec2(pieCenter.x - msSz.x * 0.5f, cTextY + cSz.y + 1.0f), kColTextMuted, "ms");
	}

	// Caption under pie.
	{
		char captionBuf[48];
		std::snprintf(captionBuf, sizeof(captionBuf), "%zu items", slices.size());
		const ImVec2 capSz = ImGui::CalcTextSize(captionBuf);
		dl->AddText(
		    ImVec2(pieCenter.x - capSz.x * 0.5f, chartTop + chartContentH - pieCaptionH),
		    kColTextDim,
		    captionBuf);
	}

	// ============ Column bar chart (right) ============
	const float plotLeft = barAreaLeft + yAxisLabelW;
	const float plotRight = barAreaRight - 2.0f;
	const float plotW = plotRight - plotLeft;
	const float plotH = chartContentH - barValLabelH; // leave room for value labels above bars
	const float plotTop = chartTop + barValLabelH;
	const float plotBot = chartTop + chartContentH;

	if (plotW > 60.0f && !barOrder.empty())
	{
		// Round axis maximum up to a nice number for tick labels.
		auto niceCeil = [](double v) -> double
		{
			if (v <= 0.0)
			{
				return 1.0;
			}
			const double mag = std::pow(10.0, std::floor(std::log10(v)));
			const double n = v / mag;
			double nice;
			if (n <= 1.0) nice = 1.0;
			else if (n <= 2.0) nice = 2.0;
			else if (n <= 5.0) nice = 5.0;
			else nice = 10.0;
			return nice * mag;
		};
		const double axisMax = niceCeil(maxMs);

		// Y-axis grid + tick labels (0, max/4, max/2, 3max/4, max).
		for (int g = 0; g <= 4; ++g)
		{
			const float gy = plotBot - plotH * (static_cast<float>(g) / 4.0f);
			const ImU32 col = (g == 0) ? kColAxis : kColGrid;
			dl->AddLine(ImVec2(plotLeft, gy), ImVec2(plotRight, gy), col);

			char tickBuf[16];
			std::snprintf(tickBuf, sizeof(tickBuf), "%.1f", axisMax * (g / 4.0));
			const ImVec2 tSz = ImGui::CalcTextSize(tickBuf);
			dl->AddText(ImVec2(plotLeft - tSz.x - 4.0f, gy - tSz.y * 0.5f), kColTextDim, tickBuf);
		}

		// Bar geometry — cap bar width so they don't look like squares.
		const float maxBarW = 40.0f;
		const float minBarSpacing = 6.0f;
		const float numBars = static_cast<float>(barOrder.size());
		float slotW = plotW / numBars;
		float barW = std::min(maxBarW, slotW - minBarSpacing);
		barW = std::max(barW, 8.0f);

		// Center the group of bars within the plot if they don't fill it.
		const float groupW = numBars * barW + (numBars - 1.0f) * minBarSpacing;
		const float plotPadLeft = std::max(0.0f, (plotW - groupW) * 0.5f);

		for (std::size_t bi = 0; bi < barOrder.size(); ++bi)
		{
			const Slice& s = slices[barOrder[bi]];
			const float xL = plotLeft + plotPadLeft + static_cast<float>(bi) * (barW + minBarSpacing);
			const float xR = xL + barW;
			const float frac = axisMax > 0.0 ? static_cast<float>(s.ValueMs / axisMax) : 0.0f;
			const float barH = plotH * frac;
			const float bTop = plotBot - barH;
			const float bBot = plotBot;

			// Bar — flat color with a slightly darker top accent line.
			const ImU32 col = s.Color;
			ImVec4 colTopRgb = ImGui::ColorConvertU32ToFloat4(col);
			colTopRgb.x = std::min(1.0f, colTopRgb.x * 1.15f);
			colTopRgb.y = std::min(1.0f, colTopRgb.y * 1.15f);
			colTopRgb.z = std::min(1.0f, colTopRgb.z * 1.15f);
			const ImU32 colTopAccent = ImGui::ColorConvertFloat4ToU32(colTopRgb);
			dl->AddRectFilled(ImVec2(xL, bTop), ImVec2(xR, bBot), col, 1.5f, ImDrawFlags_RoundCornersTop);
			if (barH > 4.0f)
			{
				dl->AddLine(ImVec2(xL + 1.0f, bTop + 1.0f), ImVec2(xR - 1.0f, bTop + 1.0f), colTopAccent, 1.0f);
			}

			// Value above bar.
			char vBuf[16];
			std::snprintf(vBuf, sizeof(vBuf), "%.2f", s.ValueMs);
			const ImVec2 vSz = ImGui::CalcTextSize(vBuf);
			const float vCenterX = xL + barW * 0.5f;
			const float vY = std::max(plotTop - vSz.y - 1.0f, bTop - vSz.y - 2.0f);
			dl->AddText(ImVec2(vCenterX - vSz.x * 0.5f, vY), kColTextStrong, vBuf);

			// Name below bar — rotated 35° anchored at the bar's bottom-center.
			{
				const ImVec2 anchor{xL + barW * 0.5f, bBot + 4.0f};
				const int vtxStart = dl->VtxBuffer.Size;
				dl->AddText(anchor, kColTextMuted, s.ShortName.c_str());
				const int vtxEnd = dl->VtxBuffer.Size;
				const float c = std::cos(labelAngleRad);
				const float si = std::sin(labelAngleRad);
				for (int vi = vtxStart; vi < vtxEnd; ++vi)
				{
					ImDrawVert& v = dl->VtxBuffer[vi];
					const float dx = v.pos.x - anchor.x;
					const float dy = v.pos.y - anchor.y;
					v.pos.x = anchor.x + dx * c - dy * si;
					v.pos.y = anchor.y + dx * si + dy * c;
				}
			}

			// Hover hit area covers the entire column slot.
			const float hitL = xL - minBarSpacing * 0.5f;
			const float hitR = xR + minBarSpacing * 0.5f;
			if (ImGui::IsMouseHoveringRect(ImVec2(hitL, plotTop), ImVec2(hitR, plotBot + barXLabelH)))
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s", s.ShortName.c_str());
				ImGui::Separator();
				ImGui::Text("%.3f ms  (%.1f%%)", s.ValueMs, s.Pct);
				ImGui::EndTooltip();
			}
		}
	}

	// ============ Footer ============
	{
		const float footY = cardOrigin.y + cardH - outerPad - footerH + 2.0f;
		dl->AddLine(
		    ImVec2(cx, footY - innerPad * 0.5f),
		    ImVec2(cx + innerWidth, footY - innerPad * 0.5f),
		    kColDivider);

		char totalBuf[64];
		std::snprintf(totalBuf, sizeof(totalBuf), "Total inclusive: %.3f ms", totalMs);
		dl->AddText(ImVec2(cx, footY), kColTextMuted, totalBuf);

		char rightBuf[64];
		std::snprintf(rightBuf, sizeof(rightBuf), "Peak: %.3f ms  ·  Avg: %.3f ms", maxMs, totalMs / static_cast<double>(slices.size()));
		const ImVec2 rSz = ImGui::CalcTextSize(rightBuf);
		dl->AddText(ImVec2(cx + innerWidth - rSz.x, footY), kColTextDim, rightBuf);
	}

	ImGui::Dummy(ImVec2(availWidth, cardH));
	ImGui::EndChild();
	ImGui::PopID();
}
