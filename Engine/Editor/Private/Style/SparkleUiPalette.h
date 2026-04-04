#pragma once

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
}  // namespace SparkleUiPalette