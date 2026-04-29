#include "PCH.h"
#include "Panels/Profiler/ProfilerChartView.h"

#include "Panels/Profiler/ProfilerSnapshotUtils.h"
#include "Style/SparkleUiPalette.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

double ProfilerChartView::NiceCeil(double value) noexcept
{
	if (value <= 0.0)
	{
		return 1.0;
	}
	const double mag = std::pow(10.0, std::floor(std::log10(value)));
	const double n = value / mag;
	double nice = 10.0;
	if (n <= 1.0)
	{
		nice = 1.0;
	}
	else if (n <= 2.0)
	{
		nice = 2.0;
	}
	else if (n <= 5.0)
	{
		nice = 5.0;
	}
	return nice * mag;
}

bool ProfilerChartView::IsScopeHidden(
    const Diagnostics::ProfilerSnapshotNode& node,
    const std::unordered_set<std::string>& hiddenScopes) noexcept
{
	return hiddenScopes.count(node.Name) > 0;
}

double ProfilerChartView::ComputeHiddenDescendantAverageMicroseconds(
    const Diagnostics::ProfilerSnapshotNode& node,
    const std::unordered_set<std::string>& hiddenScopes) noexcept
{
	double hiddenMicroseconds = 0.0;
	for (const Diagnostics::ProfilerSnapshotNode& child : node.Children)
	{
		if (IsScopeHidden(child, hiddenScopes))
		{
			hiddenMicroseconds += child.AverageDurationMicroseconds;
			continue;
		}

		hiddenMicroseconds += ComputeHiddenDescendantAverageMicroseconds(child, hiddenScopes);
	}
	return hiddenMicroseconds;
}

double ProfilerChartView::ComputeVisibleAverageMicroseconds(
    const Diagnostics::ProfilerSnapshotNode& node,
    const std::unordered_set<std::string>& hiddenScopes) noexcept
{
	if (IsScopeHidden(node, hiddenScopes))
	{
		return 0.0;
	}

	const double hiddenDescendantMicroseconds = ComputeHiddenDescendantAverageMicroseconds(node, hiddenScopes);
	return std::max(0.0, node.AverageDurationMicroseconds - hiddenDescendantMicroseconds);
}

bool ProfilerChartView::BuildSlices(
    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
    const std::unordered_set<std::string>& hiddenScopes,
    std::vector<Slice>& outSlices,
    double& outTotalMs,
    double& outMaxMs)
{
	outSlices.clear();
	outSlices.reserve(bucket.size());
	outTotalMs = 0.0;
	outMaxMs = 0.0;
	for (std::size_t i = 0; i < bucket.size(); ++i)
	{
		const Diagnostics::ProfilerSnapshotNode* node = bucket[i];
		if (IsScopeHidden(*node, hiddenScopes))
		{
			continue;
		}
		const double visibleAverageMicroseconds = ComputeVisibleAverageMicroseconds(*node, hiddenScopes);
		if (visibleAverageMicroseconds <= 0.0)
		{
			continue;
		}
		const double ms = ProfilerSnapshotUtils::MicrosecondsToMilliseconds(visibleAverageMicroseconds);
		const std::string_view sv = ProfilerSnapshotUtils::ShortenScopeName(node->Name);
		// Slice color is derived from the node's index *in the original bucket*
		// so chart slices match the table's row tint and visibility-dot colors.
		const ImU32 color = SparkleUiPalette::CategoricalColor(i);
		outSlices.push_back(Slice{std::string(sv), ms, 0.0, color});
		outTotalMs += ms;
		outMaxMs = std::max(outMaxMs, ms);
	}
	return outTotalMs > 0.0;
}

bool ProfilerChartView::Render(
    const std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
    std::string_view moduleName,
    const std::unordered_set<std::string>& hiddenScopes) const
{
	std::vector<Slice> slices;
	double totalMs = 0.0;
	double maxMs = 0.0;
	if (!BuildSlices(bucket, hiddenScopes, slices, totalMs, maxMs))
	{
		return false;
	}
	for (Slice& s : slices)
	{
		s.Pct = s.ValueMs / totalMs * 100.0;
	}

	// Sorted indices for the bar chart (most expensive â†’ cheapest). Pie keeps
	// insertion order so its slices line up with table rows.
	std::vector<std::size_t> barOrder(slices.size());
	std::iota(barOrder.begin(), barOrder.end(), std::size_t{0});
	std::sort(
	    barOrder.begin(),
	    barOrder.end(),
	    [&slices](std::size_t a, std::size_t b) { return slices[a].ValueMs > slices[b].ValueMs; });

	const float availWidth = ImGui::GetContentRegionAvail().x;
	if (availWidth < kMinChartWidth)
	{
		return false;
	}

	// ---- Palette (all colors come from SparkleUiPalette so the chart matches
	//      the rest of the editor's dark theme) ----
	const ImU32 colCardBg = SparkleUiPalette::ChartCardBackground();
	const ImU32 colCardBorder = SparkleUiPalette::ChartCardBorder();
	const ImU32 colDivider = SparkleUiPalette::ChartDivider();
	const ImU32 colAxis = SparkleUiPalette::ChartAxis();
	const ImU32 colGrid = SparkleUiPalette::ChartGrid();
	const ImU32 colTextStrong = SparkleUiPalette::ChartTextStrong();
	const ImU32 colTextMuted = SparkleUiPalette::ChartTextMuted();
	const ImU32 colTextDim = SparkleUiPalette::ChartTextDim();
	const ImU32 colTitle = SparkleUiPalette::ChartTitle();

	// ---- Layout ----
	const float fontSize = ImGui::GetFontSize();
	const float rowH = fontSize + 3.0f;
	const float innerWidth = availWidth - kOuterPadding * 2.0f;
	const float titleBarH = rowH + 2.0f;

	const float piePanelW = std::min(innerWidth * kPieWidthFraction, kPiePanelMaxWidth);
	const float pieMaxSize = std::min(piePanelW - 24.0f, kPieMaxDiameter);
	const float pieRadius = pieMaxSize * 0.5f;
	const float donutHole = pieRadius * kDonutHoleFraction;
	const float pieCaptionH = fontSize + 4.0f;

	const float yAxisLabelW = ImGui::CalcTextSize("88.8").x + 6.0f;
	const float barValLabelH = fontSize + 2.0f;

	float maxLabelW = 0.0f;
	for (const Slice& s : slices)
	{
		maxLabelW = std::max(maxLabelW, ImGui::CalcTextSize(s.ShortName.c_str()).x);
	}
	const float labelAngleRad = kLabelAngleDegrees * (kPi / 180.0f);
	const float barXLabelH = maxLabelW * std::sin(labelAngleRad) + fontSize * std::cos(labelAngleRad) + 6.0f;

	const float chartContentH = std::max(pieMaxSize + pieCaptionH, 150.0f);
	const float footerH = rowH + 2.0f;
	const float cardContentH = titleBarH + kInnerPadding + chartContentH + barXLabelH + kInnerPadding + footerH;
	const float cardH = cardContentH + kOuterPadding * 2.0f;

	// ---- Card container ----
	ImGui::PushID(moduleName.data() != nullptr ? moduleName.data() : "charts");
	ImGui::BeginChild("##Charts", ImVec2(availWidth, cardH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 cardOrigin = ImGui::GetCursorScreenPos();

	// Background â€” round only the bottom corners so the top edge butts cleanly
	// against the table border above the chart.
	dl->AddRectFilled(
	    cardOrigin,
	    ImVec2(cardOrigin.x + availWidth, cardOrigin.y + cardH),
	    colCardBg,
	    kCardCornerRadius,
	    ImDrawFlags_RoundCornersBottom);

	const float cx = cardOrigin.x + kOuterPadding;
	float cy = cardOrigin.y + kOuterPadding;

	// Header band.
	const float pieAreaLeft = cx;
	const float pieAreaRight = cx + piePanelW;
	const float dividerX = pieAreaRight + kInnerPadding * 0.5f;
	const float barAreaLeft = pieAreaRight + kInnerPadding;
	const float barAreaRight = cx + innerWidth;

	dl->AddText(ImVec2(pieAreaLeft, cy), colTitle, "Distribution");
	dl->AddText(ImVec2(barAreaLeft, cy), colTitle, "Pass timings (ms)");
	cy += titleBarH + kInnerPadding;

	const float chartTop = cy;
	const float chartBot = cy + chartContentH;

	dl->AddLine(ImVec2(dividerX, chartTop - 4.0f), ImVec2(dividerX, chartBot + barXLabelH + 2.0f), colDivider);

	// ---- Donut pie ----
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

		// Per-slice tooltip â€” uses precomputed pct so numbers always match the bar chart.
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
	dl->AddCircleFilled(pieCenter, donutHole, colCardBg, 48);
	dl->AddCircle(pieCenter, donutHole, colCardBorder, 48, 1.0f);
	dl->AddCircle(pieCenter, pieRadius, colCardBorder, 48, 1.0f);

	{
		char centerBuf[16];
		std::snprintf(centerBuf, sizeof(centerBuf), "%.2f", totalMs);
		const ImVec2 cSz = ImGui::CalcTextSize(centerBuf);
		const float cTextY = pieCenter.y - cSz.y;
		dl->AddText(ImVec2(pieCenter.x - cSz.x * 0.5f, cTextY), colTextStrong, centerBuf);
		const ImVec2 msSz = ImGui::CalcTextSize("ms");
		dl->AddText(ImVec2(pieCenter.x - msSz.x * 0.5f, cTextY + cSz.y + 1.0f), colTextMuted, "ms");
	}

	{
		char captionBuf[48];
		std::snprintf(captionBuf, sizeof(captionBuf), "%zu items", slices.size());
		const ImVec2 capSz = ImGui::CalcTextSize(captionBuf);
		dl->AddText(
		    ImVec2(pieCenter.x - capSz.x * 0.5f, chartTop + chartContentH - pieCaptionH),
		    colTextDim,
		    captionBuf);
	}

	// ---- Column bar chart ----
	const float plotLeft = barAreaLeft + yAxisLabelW;
	const float plotRight = barAreaRight - 2.0f;
	const float plotW = plotRight - plotLeft;
	const float plotH = chartContentH - barValLabelH;
	const float plotTop = chartTop + barValLabelH;
	const float plotBot = chartTop + chartContentH;

	if (plotW > kMinPlotWidth && !barOrder.empty())
	{
		const double axisMax = NiceCeil(maxMs);

		for (int g = 0; g <= kYAxisGridSteps; ++g)
		{
			const float gy = plotBot - plotH * (static_cast<float>(g) / static_cast<float>(kYAxisGridSteps));
			const ImU32 col = (g == 0) ? colAxis : colGrid;
			dl->AddLine(ImVec2(plotLeft, gy), ImVec2(plotRight, gy), col);

			char tickBuf[16];
			std::snprintf(tickBuf, sizeof(tickBuf), "%.1f", axisMax * (static_cast<double>(g) / static_cast<double>(kYAxisGridSteps)));
			const ImVec2 tSz = ImGui::CalcTextSize(tickBuf);
			dl->AddText(ImVec2(plotLeft - tSz.x - 4.0f, gy - tSz.y * 0.5f), colTextDim, tickBuf);
		}

		const float numBars = static_cast<float>(barOrder.size());
		float slotW = plotW / numBars;
		float barW = std::min(kBarMaxWidth, slotW - kBarMinSpacing);
		barW = std::max(barW, kBarMinWidth);

		const float groupW = numBars * barW + (numBars - 1.0f) * kBarMinSpacing;
		const float plotPadLeft = std::max(0.0f, (plotW - groupW) * 0.5f);

		for (std::size_t bi = 0; bi < barOrder.size(); ++bi)
		{
			const Slice& s = slices[barOrder[bi]];
			const float xL = plotLeft + plotPadLeft + static_cast<float>(bi) * (barW + kBarMinSpacing);
			const float xR = xL + barW;
			const float frac = axisMax > 0.0 ? static_cast<float>(s.ValueMs / axisMax) : 0.0f;
			const float barH = plotH * frac;
			const float bTop = plotBot - barH;
			const float bBot = plotBot;

			const ImU32 col = s.Color;
			ImVec4 colTopRgb = ImGui::ColorConvertU32ToFloat4(col);
			colTopRgb.x = std::min(1.0f, colTopRgb.x * 1.15f);
			colTopRgb.y = std::min(1.0f, colTopRgb.y * 1.15f);
			colTopRgb.z = std::min(1.0f, colTopRgb.z * 1.15f);
			const ImU32 colTopAccent = ImGui::ColorConvertFloat4ToU32(colTopRgb);
			dl->AddRectFilled(ImVec2(xL, bTop), ImVec2(xR, bBot), col, kBarCornerRadius, ImDrawFlags_RoundCornersTop);
			if (barH > 4.0f)
			{
				dl->AddLine(ImVec2(xL + 1.0f, bTop + 1.0f), ImVec2(xR - 1.0f, bTop + 1.0f), colTopAccent, 1.0f);
			}

			char vBuf[16];
			std::snprintf(vBuf, sizeof(vBuf), "%.2f", s.ValueMs);
			const ImVec2 vSz = ImGui::CalcTextSize(vBuf);
			const float vCenterX = xL + barW * 0.5f;
			const float vY = std::max(plotTop - vSz.y - 1.0f, bTop - vSz.y - 2.0f);
			dl->AddText(ImVec2(vCenterX - vSz.x * 0.5f, vY), colTextStrong, vBuf);

			// Rotated x-axis label, anchored at the bar's bottom-center. We rotate
			// the glyph vertices in place so the text stays crisp at any angle.
			{
				const ImVec2 anchor{xL + barW * 0.5f, bBot + 4.0f};
				const int vtxStart = dl->VtxBuffer.Size;
				dl->AddText(anchor, colTextMuted, s.ShortName.c_str());
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

			const float hitL = xL - kBarMinSpacing * 0.5f;
			const float hitR = xR + kBarMinSpacing * 0.5f;
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

	// ---- Footer ----
	{
		const float footY = cardOrigin.y + cardH - kOuterPadding - footerH + 2.0f;
		dl->AddLine(
		    ImVec2(cx, footY - kInnerPadding * 0.5f),
		    ImVec2(cx + innerWidth, footY - kInnerPadding * 0.5f),
		    colDivider);

		char totalBuf[64];
		std::snprintf(totalBuf, sizeof(totalBuf), "Total inclusive: %.3f ms", totalMs);
		dl->AddText(ImVec2(cx, footY), colTextMuted, totalBuf);

		char rightBuf[64];
		std::snprintf(rightBuf, sizeof(rightBuf), "Peak: %.3f ms  \xC2\xB7  Avg: %.3f ms", maxMs, totalMs / static_cast<double>(slices.size()));
		const ImVec2 rSz = ImGui::CalcTextSize(rightBuf);
		dl->AddText(ImVec2(cx + innerWidth - rSz.x, footY), colTextDim, rightBuf);
	}

	ImGui::Dummy(ImVec2(availWidth, cardH));
	ImGui::EndChild();
	ImGui::PopID();
	return true;
}
