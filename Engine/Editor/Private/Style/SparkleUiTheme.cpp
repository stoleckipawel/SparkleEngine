#include "PCH.h"
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
	}  // namespace

	void ApplyEditorialDarkTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(8.0f, 4.0f);
		style.CellPadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 2.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 12.0f;
		style.ScrollbarSize = 12.0f;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.86f, 0.88f, 0.91f, 1.0f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.56f, 0.59f, 0.64f, 1.0f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 0.99f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.14f, 0.99f);
		colors[ImGuiCol_Border] = ImVec4(0.23f, 0.24f, 0.27f, 1.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.19f, 0.20f, 0.22f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.25f, 0.28f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.30f, 0.46f, 1.0f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.09f, 0.09f, 0.10f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.17f, 0.18f, 0.20f, 1.0f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.21f, 0.23f, 0.27f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.32f, 0.46f, 1.0f);
		colors[ImGuiCol_Button] = ImVec4(0.21f, 0.22f, 0.24f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.27f, 0.30f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.32f, 0.52f, 1.0f);
		colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.26f, 0.29f, 1.0f);
		colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.21f, 0.24f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.28f, 0.41f, 1.0f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.60f, 0.95f, 1.0f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.33f, 0.60f, 0.95f, 1.0f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.71f, 1.0f, 1.0f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.18f, 0.36f, 0.60f, 0.35f);
	}

	void ConfigureTypography(float dpiScale)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();

		const float bodySize = 14.0f * dpiScale;
		const float headingSize = 15.0f * dpiScale;
		const float monoSize = 13.0f * dpiScale;

		g_bodyFont =
		    LoadFirstAvailableFont({"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/Inter-Regular.ttf", nullptr, nullptr}, bodySize);
		g_headingFont = LoadFirstAvailableFont(
		    {"C:/Windows/Fonts/seguisb.ttf", "C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/Inter-SemiBold.ttf", nullptr},
		    headingSize);
		g_monoFont = LoadFirstAvailableFont(
		    {"C:/Windows/Fonts/JetBrainsMono-Regular.ttf",
		     "C:/Windows/Fonts/JetBrainsMonoNL-Regular.ttf",
		     "C:/Windows/Fonts/consola.ttf",
		     nullptr},
		    monoSize);

		if (g_bodyFont == nullptr)
		{
			g_bodyFont = io.Fonts->AddFontDefault();
		}

		if (g_headingFont == nullptr)
		{
			g_headingFont = g_bodyFont;
		}

		if (g_monoFont == nullptr)
		{
			g_monoFont = g_bodyFont;
		}

		io.FontDefault = g_bodyFont;
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