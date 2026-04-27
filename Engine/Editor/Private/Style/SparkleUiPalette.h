#pragma once

#include <cstddef>

#include <imgui.h>

namespace SparkleUiPalette
{
	ImVec4 TextPrimary() noexcept;
	ImVec4 TextMuted() noexcept;
	ImVec4 WindowBackground() noexcept;
	ImVec4 SurfaceBackground() noexcept;
	ImVec4 PopupBackground() noexcept;
	ImVec4 Border() noexcept;
	ImVec4 FrameBackground() noexcept;
	ImVec4 FrameBackgroundHovered() noexcept;
	ImVec4 FrameBackgroundActive() noexcept;
	ImVec4 TitleBarBackground() noexcept;
	ImVec4 HeaderBackground() noexcept;
	ImVec4 HeaderBackgroundHovered() noexcept;
	ImVec4 HeaderBackgroundActive() noexcept;
	ImVec4 ButtonBackground() noexcept;
	ImVec4 ButtonBackgroundHovered() noexcept;
	ImVec4 ButtonBackgroundActive() noexcept;
	ImVec4 Separator() noexcept;
	ImVec4 TabBackground() noexcept;
	ImVec4 TabBackgroundHovered() noexcept;
	ImVec4 TabBackgroundActive() noexcept;
	ImVec4 Accent() noexcept;
	ImVec4 AccentStrong() noexcept;
	ImVec4 SelectionOverlay() noexcept;

	ImVec4 TitleBarControlBackground() noexcept;
	ImVec4 TitleBarControlBackgroundHovered() noexcept;
	ImVec4 TitleBarControlBackgroundActive() noexcept;
	ImVec4 DangerBackground() noexcept;
	ImVec4 DangerBackgroundHovered() noexcept;
	ImVec4 DangerBackgroundActive() noexcept;

	ImU32 TitleBarIcon() noexcept;
	ImU32 PanelHeaderBackground() noexcept;
	ImU32 PanelHeaderBorder() noexcept;
	ImU32 SectionHeaderBackground() noexcept;
	ImU32 SectionHeaderBorder() noexcept;
	ImU32 SceneOutlinerBadgeBackground() noexcept;
	ImU32 SceneOutlinerBadgeText() noexcept;
	ImVec4 ConsoleScrollbackBackground() noexcept;
	ImVec4 ConsoleInputBackground() noexcept;

	// ---- Chart / data-visualization palette ----
	// Shared by the profiler and any future chart-style widgets so colors stay
	// consistent with the editorial dark theme.
	ImU32 ChartCardBackground() noexcept;
	ImU32 ChartCardBorder() noexcept;
	ImU32 ChartDivider() noexcept;
	ImU32 ChartAxis() noexcept;
	ImU32 ChartGrid() noexcept;
	ImU32 ChartTextStrong() noexcept;
	ImU32 ChartTextMuted() noexcept;
	ImU32 ChartTextDim() noexcept;
	ImU32 ChartTitle() noexcept;

	// ---- Categorical color palette ----
	// Curated 10-color palette inspired by Tableau 10 / Chrome DevTools.
	// Used to color independent series (profiler scopes, chart bars, etc.).
	std::size_t CategoricalColorCount() noexcept;
	ImU32 CategoricalColor(std::size_t index) noexcept;
	// Returns the same hue with reduced saturation, useful for low-contrast tints
	// (e.g. table row backgrounds) that should not distract from foreground text.
	ImU32 CategoricalColorDesaturated(std::size_t index, float saturation) noexcept;
}  // namespace SparkleUiPalette