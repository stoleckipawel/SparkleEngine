#include "PCH.h"
#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"

#include <array>
#include <filesystem>

#include <imgui.h>

namespace SparkleUiTheme
{
	namespace
	{
		ImFont* g_bodyFont = nullptr;
		ImFont* g_headingFont = nullptr;
		ImFont* g_monoFont = nullptr;
		bool g_editorIconsAvailable = false;

		const char* GetFontAwesomeSolidPath() noexcept
		{
#ifdef SPARKLE_FONT_AWESOME_SOLID_TTF
			return SPARKLE_FONT_AWESOME_SOLID_TTF;
#else
			return nullptr;
#endif
		}

		ImFont* LoadFirstAvailableFont(const std::array<const char*, 4>& fontPaths, float sizePixels)
		{
			ImGuiIO& io = ImGui::GetIO();
			for (const char* fontPath : fontPaths)
			{
				if (fontPath == nullptr)
				{
					continue;
				}

				std::error_code errorCode;
				if (!std::filesystem::exists(fontPath, errorCode) || errorCode)
				{
					continue;
				}

				ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, sizePixels);
				if (font != nullptr)
				{
					return font;
				}
			}

			return nullptr;
		}

		void MergeEditorIconsIntoLastFont(float baseSizePixels)
		{
			const char* fontPath = GetFontAwesomeSolidPath();
			if (fontPath == nullptr || fontPath[0] == '\0')
			{
				return;
			}

			std::error_code errorCode;
			if (!std::filesystem::exists(fontPath, errorCode) || errorCode)
			{
				return;
			}

			ImGuiIO& io = ImGui::GetIO();
			static constexpr ImWchar kFontAwesomeRanges[] = {0xf000, 0xf8ff, 0};
			ImFontConfig iconConfig;
			iconConfig.MergeMode = true;
			iconConfig.PixelSnapH = true;
			iconConfig.GlyphMinAdvanceX = baseSizePixels;

			const float iconSize = baseSizePixels * 0.86f;
			if (io.Fonts->AddFontFromFileTTF(fontPath, iconSize, &iconConfig, kFontAwesomeRanges) != nullptr)
			{
				g_editorIconsAvailable = true;
			}
		}
	}  // namespace

	void ApplyEditorialDarkTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.0f, 8.0f);
		style.FramePadding = ImVec2(6.0f, 3.0f);
		style.CellPadding = ImVec2(4.0f, 3.0f);
		style.ItemSpacing = ImVec2(6.0f, 4.0f);
		style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 2.0f;
		style.WindowBorderSize = 0.0f;
		style.ChildBorderSize = 0.0f;
		style.FrameBorderSize = 0.0f;
		style.IndentSpacing = 12.0f;
		style.ScrollbarSize = 10.0f;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = SparkleUiPalette::TextPrimary();
		colors[ImGuiCol_TextDisabled] = SparkleUiPalette::TextMuted();
		colors[ImGuiCol_WindowBg] = SparkleUiPalette::WindowBackground();
		colors[ImGuiCol_ChildBg] = SparkleUiPalette::SurfaceBackground();
		colors[ImGuiCol_PopupBg] = SparkleUiPalette::PopupBackground();
		colors[ImGuiCol_Border] = SparkleUiPalette::Border();
		colors[ImGuiCol_FrameBg] = SparkleUiPalette::FrameBackground();
		colors[ImGuiCol_FrameBgHovered] = SparkleUiPalette::FrameBackgroundHovered();
		colors[ImGuiCol_FrameBgActive] = SparkleUiPalette::FrameBackgroundActive();
		colors[ImGuiCol_TitleBg] = SparkleUiPalette::TitleBarBackground();
		colors[ImGuiCol_TitleBgActive] = SparkleUiPalette::TitleBarBackground();
		colors[ImGuiCol_Header] = SparkleUiPalette::HeaderBackground();
		colors[ImGuiCol_HeaderHovered] = SparkleUiPalette::HeaderBackgroundHovered();
		colors[ImGuiCol_HeaderActive] = SparkleUiPalette::HeaderBackgroundActive();
		colors[ImGuiCol_Button] = SparkleUiPalette::ButtonBackground();
		colors[ImGuiCol_ButtonHovered] = SparkleUiPalette::ButtonBackgroundHovered();
		colors[ImGuiCol_ButtonActive] = SparkleUiPalette::ButtonBackgroundActive();
		colors[ImGuiCol_Separator] = SparkleUiPalette::Separator();
		colors[ImGuiCol_Tab] = SparkleUiPalette::TabBackground();
		colors[ImGuiCol_TabHovered] = SparkleUiPalette::TabBackgroundHovered();
		colors[ImGuiCol_TabActive] = SparkleUiPalette::TabBackgroundActive();
		colors[ImGuiCol_CheckMark] = SparkleUiPalette::Accent();
		colors[ImGuiCol_SliderGrab] = SparkleUiPalette::Accent();
		colors[ImGuiCol_SliderGrabActive] = SparkleUiPalette::AccentStrong();
		colors[ImGuiCol_TextSelectedBg] = SparkleUiPalette::SelectionOverlay();
	}

	void ConfigureTypography(float dpiScale)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		g_editorIconsAvailable = false;

		const float bodySize = 14.0f * dpiScale;
		const float headingSize = 15.0f * dpiScale;
		const float monoSize = 13.0f * dpiScale;

		g_bodyFont =
		    LoadFirstAvailableFont({"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/Inter-Regular.ttf", nullptr, nullptr}, bodySize);
		if (g_bodyFont == nullptr)
		{
			g_bodyFont = io.Fonts->AddFontDefault();
		}
		MergeEditorIconsIntoLastFont(bodySize);

		g_headingFont = LoadFirstAvailableFont(
		    {"C:/Windows/Fonts/seguisb.ttf", "C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/Inter-SemiBold.ttf", nullptr},
		    headingSize);
		if (g_headingFont != nullptr && g_headingFont != g_bodyFont)
		{
			MergeEditorIconsIntoLastFont(headingSize);
		}

		if (g_headingFont == nullptr)
		{
			g_headingFont = g_bodyFont;
		}

		g_monoFont = LoadFirstAvailableFont(
		    {"C:/Windows/Fonts/JetBrainsMono-Regular.ttf",
		     "C:/Windows/Fonts/JetBrainsMonoNL-Regular.ttf",
		     "C:/Windows/Fonts/consola.ttf",
		     nullptr},
		    monoSize);
		if (g_monoFont != nullptr && g_monoFont != g_bodyFont)
		{
			MergeEditorIconsIntoLastFont(monoSize);
		}

		if (g_monoFont == nullptr)
		{
			g_monoFont = g_bodyFont;
		}

		io.FontDefault = g_bodyFont;
	}

	bool AreEditorIconsAvailable() noexcept
	{
		return g_editorIconsAvailable;
	}

	ImFont* GetBodyFont()
	{
		return g_bodyFont;
	}

	ImFont* GetHeadingFont()
	{
		return g_headingFont;
	}

	ImFont* GetMonoFont()
	{
		return g_monoFont;
	}
}  // namespace SparkleUiTheme