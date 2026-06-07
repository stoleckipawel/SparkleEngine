#include "PCH.h"
#include "Panels/SceneInspectorPanel.h"

#include "Panels/SceneCameraInspector.h"
#include "Panels/SceneLightInspector.h"
#include "Panels/SceneMeshInspector.h"
#include "Scene/SceneObjectSelection.h"
#include "Scene/SceneObjectPresentation.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>

#include <imgui.h>

SceneInspectorPanel::SceneInspectorPanel(GameScene& gameScene, SceneObjectSelection& selection, float widthPixels) noexcept :
    m_gameScene(&gameScene), m_selection(&selection), m_widthPixels(widthPixels)
{
}

std::string SceneInspectorPanel::BuildSelectionTitle() const
{
	if (m_selection == nullptr)
	{
		return "No Selection";
	}

	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			if (m_gameScene != nullptr && m_selection->index < m_gameScene->GetCameras().GetCameraCount())
			{
				const std::string& name = m_gameScene->GetCameras().GetCameraEntries()[m_selection->index].name;
				return name.empty() ? "Camera " + std::to_string(m_selection->index + 1) : name;
			}
			return "Scene Camera";
		case SceneObjectType::Light:
			if (m_gameScene != nullptr)
			{
				if (const SceneLightDesc* light = m_gameScene->GetLighting().GetLight(m_selection->index))
				{
					return SceneObjectPresentation::BuildLightLabel(*light, m_selection->index);
				}
			}
			return "Light " + std::to_string(m_selection->index + 1);
		case SceneObjectType::Mesh:
			return "Mesh " + std::to_string(m_selection->index + 1);
		case SceneObjectType::None:
		default:
			return "No Selection";
	}
}

const char* SceneInspectorPanel::BuildSelectionSubtitle() const noexcept
{
	if (m_selection == nullptr)
	{
		return "Object";
	}

	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			return "Camera";
		case SceneObjectType::Light:
			if (m_gameScene != nullptr)
			{
				if (const SceneLightDesc* light = m_gameScene->GetLighting().GetLight(m_selection->index))
				{
					return SceneObjectPresentation::GetLightTypeLabel(light->GetKind());
				}
			}
			return "Light";
		case SceneObjectType::Mesh:
			return "Static Mesh";
		case SceneObjectType::None:
		default:
			return "Object";
	}
}

void SceneInspectorPanel::BuildSelectionHeader() noexcept
{
	const std::string title = BuildSelectionTitle();
	const char* subtitle = BuildSelectionSubtitle();
	const UiUtil::EditorIcon icon = SceneObjectPresentation::BuildSelectionIcon(m_selection, m_gameScene);

	constexpr float kHeaderHeight = 30.0f;
	constexpr float kHeaderPaddingX = 8.0f;
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 start = ImGui::GetCursorScreenPos();
	const ImVec2 end(start.x + width, start.y + kHeaderHeight);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::HeaderBackgroundActive()));
	drawList->AddRectFilled(start, ImVec2(start.x + 3.0f, end.y), ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::Accent()));
	drawList->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, start.y), UiUtil::WithAlphaU32(SparkleUiPalette::Border(), 0.52f));
	drawList->AddLine(ImVec2(start.x, end.y - 1.0f), ImVec2(end.x, end.y - 1.0f), UiUtil::WithAlphaU32(SparkleUiPalette::Border(), 0.58f));

	ImGui::InvisibleButton("##DetailsSelectionHeader", ImVec2(width, kHeaderHeight));

	ImGui::SetCursorScreenPos(ImVec2(start.x + kHeaderPaddingX + 10.0f, start.y + 7.0f));
	ImGui::TextUnformatted(UiUtil::GetEditorIconGlyph(icon));
	ImGui::SameLine(0.0f, 8.0f);
	ImGui::TextUnformatted(title.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("%s", subtitle);

	ImGui::SetCursorScreenPos(ImVec2(start.x, end.y));
	BuildDetailsToolbar();
}

void SceneInspectorPanel::BuildDetailsToolbar() noexcept
{
	char filterBuffer[128] = {};
	const std::size_t copyLength = (std::min) (m_filterText.size(), sizeof(filterBuffer) - 1);
	if (copyLength > 0)
	{
		std::copy_n(m_filterText.data(), copyLength, filterBuffer);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.058f, 0.064f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.075f, 0.080f, 0.090f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.070f, 0.095f, 0.130f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, UiUtil::WithAlphaU32(SparkleUiPalette::Border(), 0.70f));
	ImGui::SetNextItemWidth(-1.0f);
	const std::string searchHint = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Search");
	if (ImGui::InputTextWithHint("##DetailsFilter", searchHint.c_str(), filterBuffer, sizeof(filterBuffer)))
	{
		m_filterText = filterBuffer;
	}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);
}

void SceneInspectorPanel::SetWidth(float widthPixels) noexcept
{
	m_widthPixels = widthPixels;
}

void SceneInspectorPanel::SetTopInset(float topInsetPixels) noexcept
{
	m_topInsetPixels = topInsetPixels;
}

void SceneInspectorPanel::BuildSelectionInspector() noexcept
{
	BuildSelectionHeader();
	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			SceneCameraInspector::Build(*m_gameScene, m_filterText);
			break;
		case SceneObjectType::Light:
			SceneLightInspector::Build(*m_gameScene, m_selection->index, m_filterText);
			break;
		case SceneObjectType::Mesh:
			SceneMeshInspector::Build(*m_gameScene, m_selection->index, m_filterText);
			break;
		case SceneObjectType::None:
		default:
			UiUtil::DrawDetailsEmptyState();
			break;
	}
}

void SceneInspectorPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	constexpr float kMinWidth = 320.0f;
	const float kMaxWidth = (std::max) (kMinWidth + 1.0f, io.DisplaySize.x * 0.7f);
	m_widthPixels = std::clamp(m_widthPixels, kMinWidth, kMaxWidth);
	const float panelHeight = (std::max) (1.0f, io.DisplaySize.y - m_topInsetPixels);

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_widthPixels, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, panelHeight), ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, panelHeight), ImVec2(kMaxWidth, panelHeight));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(
	    "Inspector",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar();

	m_widthPixels = ImGui::GetWindowWidth();

	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		ImGui::TextDisabled("Scene inspector unavailable");
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);

	constexpr float kContentPad = 8.0f;
	ImGui::Indent(kContentPad);

	constexpr ImGuiTabBarFlags kTabBarFlags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyResizeDown;
	if (ImGui::BeginTabBar("##InspectorTabs", kTabBarFlags))
	{
		if (ImGui::BeginTabItem("Details"))
		{
			ImGui::Spacing();
			BuildSelectionInspector();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Unindent(kContentPad);

	ImGui::EndDisabled();

	ImGui::End();
}
