#include "PCH.h"
#include "Style/SparkleUiPalette.h"

namespace SparkleUiPalette
{
	ImVec4 TextPrimary() noexcept
	{
		return ImVec4(0.86f, 0.88f, 0.91f, 1.0f);
	}

	ImVec4 TextMuted() noexcept
	{
		return ImVec4(0.56f, 0.59f, 0.64f, 1.0f);
	}

	ImVec4 WindowBackground() noexcept
	{
		return ImVec4(0.11f, 0.11f, 0.12f, 1.0f);
	}

	ImVec4 SurfaceBackground() noexcept
	{
		return ImVec4(0.14f, 0.14f, 0.15f, 1.0f);
	}

	ImVec4 PopupBackground() noexcept
	{
		return ImVec4(0.13f, 0.13f, 0.14f, 1.0f);
	}

	ImVec4 Border() noexcept
	{
		return ImVec4(0.23f, 0.24f, 0.27f, 1.0f);
	}

	ImVec4 FrameBackground() noexcept
	{
		return ImVec4(0.19f, 0.20f, 0.22f, 1.0f);
	}

	ImVec4 FrameBackgroundHovered() noexcept
	{
		return ImVec4(0.23f, 0.25f, 0.28f, 1.0f);
	}

	ImVec4 FrameBackgroundActive() noexcept
	{
		return ImVec4(0.19f, 0.30f, 0.46f, 1.0f);
	}

	ImVec4 TitleBarBackground() noexcept
	{
		return ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
	}

	ImVec4 HeaderBackground() noexcept
	{
		return ImVec4(0.17f, 0.18f, 0.20f, 1.0f);
	}

	ImVec4 HeaderBackgroundHovered() noexcept
	{
		return ImVec4(0.21f, 0.23f, 0.27f, 1.0f);
	}

	ImVec4 HeaderBackgroundActive() noexcept
	{
		return ImVec4(0.23f, 0.32f, 0.46f, 1.0f);
	}

	ImVec4 ButtonBackground() noexcept
	{
		return ImVec4(0.21f, 0.22f, 0.24f, 1.0f);
	}

	ImVec4 ButtonBackgroundHovered() noexcept
	{
		return ImVec4(0.25f, 0.27f, 0.30f, 1.0f);
	}

	ImVec4 ButtonBackgroundActive() noexcept
	{
		return ImVec4(0.16f, 0.32f, 0.52f, 1.0f);
	}

	ImVec4 Separator() noexcept
	{
		return ImVec4(0.25f, 0.26f, 0.29f, 1.0f);
	}

	ImVec4 TabBackground() noexcept
	{
		return ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
	}

	ImVec4 TabBackgroundHovered() noexcept
	{
		return ImVec4(0.20f, 0.21f, 0.24f, 1.0f);
	}

	ImVec4 TabBackgroundActive() noexcept
	{
		return ImVec4(0.20f, 0.28f, 0.41f, 1.0f);
	}

	ImVec4 Accent() noexcept
	{
		return ImVec4(0.33f, 0.60f, 0.95f, 1.0f);
	}

	ImVec4 AccentStrong() noexcept
	{
		return ImVec4(0.46f, 0.71f, 1.0f, 1.0f);
	}

	ImVec4 SelectionOverlay() noexcept
	{
		return ImVec4(0.18f, 0.36f, 0.60f, 0.35f);
	}

	ImVec4 TitleBarControlBackground() noexcept
	{
		return ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
	}

	ImVec4 TitleBarControlBackgroundHovered() noexcept
	{
		return ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
	}

	ImVec4 TitleBarControlBackgroundActive() noexcept
	{
		return ImVec4(0.30f, 0.30f, 0.34f, 1.0f);
	}

	ImVec4 DangerBackground() noexcept
	{
		return ImVec4(0.18f, 0.10f, 0.10f, 1.0f);
	}

	ImVec4 DangerBackgroundHovered() noexcept
	{
		return ImVec4(0.60f, 0.16f, 0.16f, 1.0f);
	}

	ImVec4 DangerBackgroundActive() noexcept
	{
		return ImVec4(0.78f, 0.22f, 0.22f, 1.0f);
	}

	ImU32 TitleBarIcon() noexcept
	{
		return IM_COL32(235, 235, 235, 255);
	}

	ImU32 PanelHeaderBackground() noexcept
	{
		return IM_COL32(34, 34, 37, 255);
	}

	ImU32 PanelHeaderBorder() noexcept
	{
		return IM_COL32(72, 72, 78, 255);
	}

	ImU32 SectionHeaderBackground() noexcept
	{
		return IM_COL32(44, 46, 50, 255);
	}

	ImU32 SectionHeaderBorder() noexcept
	{
		return IM_COL32(70, 74, 80, 255);
	}

	ImU32 SceneOutlinerBadgeBackground() noexcept
	{
		return IM_COL32(58, 64, 74, 220);
	}

	ImU32 SceneOutlinerBadgeText() noexcept
	{
		return IM_COL32(220, 224, 230, 255);
	}

	ImVec4 ConsoleScrollbackBackground() noexcept
	{
		return ImVec4(0.10f, 0.11f, 0.12f, 1.0f);
	}

	ImVec4 ConsoleInputBackground() noexcept
	{
		return ImVec4(0.13f, 0.15f, 0.17f, 1.0f);
	}

	// ---- Chart / data-visualization palette ----

	ImU32 ChartCardBackground() noexcept
	{
		return IM_COL32(24, 25, 30, 230);
	}

	ImU32 ChartCardBorder() noexcept
	{
		return IM_COL32(48, 50, 58, 140);
	}

	ImU32 ChartDivider() noexcept
	{
		return IM_COL32(48, 50, 58, 140);
	}

	ImU32 ChartAxis() noexcept
	{
		return IM_COL32(70, 72, 82, 200);
	}

	ImU32 ChartGrid() noexcept
	{
		return IM_COL32(50, 52, 60, 90);
	}

	ImU32 ChartTextStrong() noexcept
	{
		return IM_COL32(220, 222, 228, 230);
	}

	ImU32 ChartTextMuted() noexcept
	{
		return IM_COL32(140, 144, 156, 200);
	}

	ImU32 ChartTextDim() noexcept
	{
		return IM_COL32(105, 110, 122, 200);
	}

	ImU32 ChartTitle() noexcept
	{
		return IM_COL32(170, 175, 190, 220);
	}

	// ---- Categorical color palette ----
	// Inspired by Tableau 10 / Chrome DevTools — muted but distinguishable on dark backgrounds.
	static constexpr ImU32 kCategoricalPalette[] = {
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

	std::size_t CategoricalColorCount() noexcept
	{
		return sizeof(kCategoricalPalette) / sizeof(kCategoricalPalette[0]);
	}

	ImU32 CategoricalColor(std::size_t index) noexcept
	{
		return kCategoricalPalette[index % CategoricalColorCount()];
	}

	ImU32 CategoricalColorDesaturated(std::size_t index, float saturation) noexcept
	{
		const ImU32 base = CategoricalColor(index);
		if (saturation >= 0.999f)
		{
			return base;
		}
		ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(base);
		float h = 0.0f;
		float s = 0.0f;
		float v = 0.0f;
		ImGui::ColorConvertRGBtoHSV(rgb.x, rgb.y, rgb.z, h, s, v);
		s *= saturation;
		ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
		rgb.w = 1.0f;
		return ImGui::ColorConvertFloat4ToU32(rgb);
	}
}
