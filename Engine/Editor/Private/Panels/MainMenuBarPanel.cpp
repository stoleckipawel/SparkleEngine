#include "PCH.h"
#include "Panels/MainMenuBarPanel.h"

#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "Style/SparkleUiPalette.h"
#include "Window/Window.h"

#include <imgui.h>
#include <cstdio>
#include <utility>

MainMenuBarPanel::MainMenuBarPanel(LevelManager* levelManager, Window* window) noexcept
{
	SetLevelManager(levelManager);
	SetWindow(window);
}

bool MainMenuBarPanel::DrawTitleBarButton(
    const char* id,
    const ImVec2& size,
    const ImVec4& baseColor,
    const ImVec4& hoveredColor,
    const ImVec4& activeColor) noexcept
{
	ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
	const bool pressed = ImGui::Button(id, size);
	ImGui::PopStyleColor(3);
	return pressed;
}

void MainMenuBarPanel::DrawMinimizeIcon() const noexcept
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 iconColor = SparkleUiPalette::TitleBarIcon();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const float y = min.y + ((max.y - min.y) * 0.68f);
	const float padding = (max.x - min.x) * 0.30f;
	drawList->AddLine(ImVec2(min.x + padding, y), ImVec2(max.x - padding, y), iconColor, 1.6f);
}

void MainMenuBarPanel::DrawMaximizeIcon() const noexcept
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 iconColor = SparkleUiPalette::TitleBarIcon();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const float width = max.x - min.x;
	const float padding = width * 0.28f;
	const ImVec2 topLeft(min.x + padding, min.y + padding);
	const ImVec2 bottomRight(max.x - padding, max.y - padding);

	if (!m_window->IsMaximized())
	{
		drawList->AddRect(topLeft, bottomRight, iconColor, 0.0f, 0, 1.4f);
		return;
	}

	const float offset = width * 0.14f;
	drawList->AddRect(ImVec2(topLeft.x + offset, topLeft.y), ImVec2(bottomRight.x, bottomRight.y - offset), iconColor, 0.0f, 0, 1.2f);
	drawList->AddRect(ImVec2(topLeft.x, topLeft.y + offset), ImVec2(bottomRight.x - offset, bottomRight.y), iconColor, 0.0f, 0, 1.2f);
}

void MainMenuBarPanel::DrawCloseIcon() const noexcept
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 iconColor = SparkleUiPalette::TitleBarIcon();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const float padding = (max.x - min.x) * 0.30f;
	drawList->AddLine(ImVec2(min.x + padding, min.y + padding), ImVec2(max.x - padding, max.y - padding), iconColor, 1.6f);
	drawList->AddLine(ImVec2(max.x - padding, min.y + padding), ImVec2(min.x + padding, max.y - padding), iconColor, 1.6f);
}

void MainMenuBarPanel::SetLevelManager(LevelManager* levelManager) noexcept
{
	m_levelManager = levelManager;
}

void MainMenuBarPanel::SetWindow(Window* window) noexcept
{
	m_window = window;
}

void MainMenuBarPanel::SetShaderReloadRequestHandler(std::function<void()> handler)
{
	m_shaderReloadRequestHandler = std::move(handler);
}

void MainMenuBarPanel::SetShaderRecookRequestHandler(std::function<void()> handler)
{
	m_shaderRecookRequestHandler = std::move(handler);
}

void MainMenuBarPanel::SetShaderInspectorOpenHandler(std::function<void()> handler)
{
	m_shaderInspectorOpenHandler = std::move(handler);
}

void MainMenuBarPanel::SetShaderPackageGenerationProvider(std::function<std::uint64_t()> provider)
{
	m_shaderPackageGenerationProvider = std::move(provider);
}

void MainMenuBarPanel::BuildOpenLevelMenu() noexcept
{
	if (m_levelManager == nullptr)
	{
		ImGui::MenuItem("No level manager", nullptr, false, false);
		return;
	}

	std::vector<std::string> levelNames = m_levelManager->GetRegisteredLevelNames();
	if (levelNames.empty())
	{
		ImGui::MenuItem("No levels found", nullptr, false, false);
		return;
	}

	const LevelAsset* activeLevel = m_levelManager->GetActiveLevel();
	const std::string activeLevelName = activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string();

	for (const std::string& levelName : levelNames)
	{
		const bool isActive = levelName == activeLevelName;
		if (ImGui::MenuItem(levelName.c_str(), nullptr, isActive, !isActive))
		{
			m_levelManager->RequestLevelChange(levelName);
		}
	}
}

void MainMenuBarPanel::BuildFileMenu() noexcept
{
	const bool hasLevelManager = m_levelManager != nullptr;
	const bool hasActiveLevel = hasLevelManager && m_levelManager->HasActiveLevel();
	const bool levelChangeInProgress = hasLevelManager && m_levelManager->IsLevelChangeInProgress();

	if (ImGui::BeginMenu("Open Level", hasLevelManager && !levelChangeInProgress))
	{
		BuildOpenLevelMenu();
		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Save All", nullptr, false, hasActiveLevel && !levelChangeInProgress))
	{
		m_levelManager->SaveActiveLevel();
	}
}

void MainMenuBarPanel::BuildShaderMenu() noexcept
{
	if (ImGui::MenuItem("Reload Cooked Shaders", nullptr, false, static_cast<bool>(m_shaderReloadRequestHandler)))
	{
		m_shaderReloadRequestHandler();
	}

	if (ImGui::MenuItem("Recook Shaders", nullptr, false, static_cast<bool>(m_shaderRecookRequestHandler)))
	{
		m_shaderRecookRequestHandler();
	}

	if (ImGui::MenuItem("Shader Inspector", nullptr, false, static_cast<bool>(m_shaderInspectorOpenHandler)))
	{
		m_shaderInspectorOpenHandler();
	}

	if (m_shaderPackageGenerationProvider)
	{
		ImGui::Separator();
		ImGui::TextDisabled("Runtime generation: %llu", static_cast<unsigned long long>(m_shaderPackageGenerationProvider()));
	}
}

void MainMenuBarPanel::BuildWindowControls() noexcept
{
	if (m_window == nullptr || m_window->IsFullScreen())
	{
		return;
	}

	const ImGuiStyle& style = ImGui::GetStyle();
	const float buttonHeight = ImGui::GetFrameHeight();
	const float buttonWidth = buttonHeight * 1.35f;
	const float controlsWidth = (buttonWidth * 3.0f) + (style.ItemSpacing.x * 2.0f);
	const float currentCursorX = ImGui::GetCursorPosX();
	const float rightEdgeX = ImGui::GetWindowWidth() - style.WindowPadding.x;
	float dragRegionWidth = rightEdgeX - currentCursorX - controlsWidth;
	if (dragRegionWidth < 1.0f)
	{
		dragRegionWidth = 1.0f;
	}

	ImGui::SameLine();
	ImGui::InvisibleButton("##TitleBarDragRegion", ImVec2(dragRegionWidth, buttonHeight));
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		m_window->ToggleMaximizeRestore();
	}
	else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
	{
		m_window->BeginDragMove();
		ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = false;
	}

	ImGui::SameLine(0.0f, style.ItemSpacing.x);
	if (DrawTitleBarButton(
	        "##MinimizeWindow",
	        ImVec2(buttonWidth, buttonHeight),
	        SparkleUiPalette::TitleBarControlBackground(),
	        SparkleUiPalette::TitleBarControlBackgroundHovered(),
	        SparkleUiPalette::TitleBarControlBackgroundActive()))
	{
		m_window->Minimize();
	}
	DrawMinimizeIcon();

	ImGui::SameLine(0.0f, style.ItemSpacing.x);
	if (DrawTitleBarButton(
	        "##ToggleMaximizeWindow",
	        ImVec2(buttonWidth, buttonHeight),
	        SparkleUiPalette::TitleBarControlBackground(),
	        SparkleUiPalette::TitleBarControlBackgroundHovered(),
	        SparkleUiPalette::TitleBarControlBackgroundActive()))
	{
		m_window->ToggleMaximizeRestore();
	}
	DrawMaximizeIcon();

	ImGui::SameLine(0.0f, style.ItemSpacing.x);
	if (DrawTitleBarButton(
	        "##CloseWindow",
	        ImVec2(buttonWidth, buttonHeight),
	        SparkleUiPalette::DangerBackground(),
	        SparkleUiPalette::DangerBackgroundHovered(),
	        SparkleUiPalette::DangerBackgroundActive()))
	{
		m_window->RequestClose();
	}
	DrawCloseIcon();
}

void MainMenuBarPanel::BuildUI() noexcept
{
	m_heightPixels = 0.0f;

	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	m_heightPixels = ImGui::GetWindowSize().y;

	if (ImGui::BeginMenu("File"))
	{
		BuildFileMenu();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Shaders"))
	{
		BuildShaderMenu();
		ImGui::EndMenu();
	}

	if (m_levelManager != nullptr)
	{
		const LevelAsset* activeLevel = m_levelManager->GetActiveLevel();
		const std::string activeLevelName = activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string("<None>");

		ImGui::Separator();
		ImGui::TextDisabled("Level");
		ImGui::SameLine();
		ImGui::TextUnformatted(activeLevelName.c_str());
	}

	{
		const ImGuiIO& io = ImGui::GetIO();
		char statsText[64] = {};
		std::snprintf(statsText, sizeof(statsText), "%.1f FPS  %.2f ms", io.Framerate, io.DeltaTime * 1000.0f);

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::TextDisabled("%s", statsText);
	}

	BuildWindowControls();

	ImGui::EndMainMenuBar();
}
