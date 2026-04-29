#include "PCH.h"
#include "Panels/SceneOutlinerPanel.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Scene/SceneObjectSelection.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>

#include <imgui.h>

SceneOutlinerPanel::SceneOutlinerPanel(GameScene& gameScene, SceneObjectSelection& selection, float widthPixels) noexcept :
    m_gameScene(&gameScene), m_selection(&selection), m_widthPixels(widthPixels)
{
}

std::string SceneOutlinerPanel::BuildMeshLabel(std::size_t meshIndex)
{
	return "Mesh " + std::to_string(meshIndex + 1);
}

void SceneOutlinerPanel::SetWidth(float widthPixels) noexcept
{
	m_widthPixels = widthPixels;
}

void SceneOutlinerPanel::SetTopInset(float topInsetPixels) noexcept
{
	m_topInsetPixels = topInsetPixels;
}

void SceneOutlinerPanel::BuildToolbar() noexcept
{
	char filterBuffer[128] = {};
	const std::size_t copyLength = (std::min) (m_filterText.size(), sizeof(filterBuffer) - 1);
	if (copyLength > 0)
	{
		std::copy_n(m_filterText.data(), copyLength, filterBuffer);
	}

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##SceneFilter", "Search actors, lights, meshes...", filterBuffer, sizeof(filterBuffer)))
	{
		m_filterText = filterBuffer;
	}
}

bool SceneOutlinerPanel::IsSelectionValid() const noexcept
{
	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		return false;
	}

	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			return true;
		case SceneObjectType::DirectionalLight:
			return m_selection->index < m_gameScene->GetLighting().GetDirectionalLightCount();
		case SceneObjectType::Mesh:
			return m_selection->index < m_gameScene->GetMeshes().GetMeshCount();
		case SceneObjectType::None:
		default:
			return false;
	}
}

void SceneOutlinerPanel::EnsureValidSelection() noexcept
{
	if (m_selection == nullptr)
	{
		return;
	}

	if (!IsSelectionValid())
	{
		*m_selection = SceneObjectSelection::Camera();
	}
}

void SceneOutlinerPanel::BuildCameraSection() noexcept
{
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Camera", flags, "Camera (1)"))
	{
		if (m_filterText.empty() || Strings::ContainsIgnoreCase("Scene Camera", m_filterText))
		{
			DrawSelectionEntry("Scene Camera", "C", "Camera", SceneObjectSelection::Camera());
		}
		ImGui::TreePop();
	}
}

void SceneOutlinerPanel::BuildLightSection() noexcept
{
	const std::size_t lightCount = m_gameScene->GetLighting().GetDirectionalLightCount();
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	const std::string header = "Lights (" + std::to_string(lightCount) + ")";
	if (ImGui::TreeNodeEx("Lights", flags, "%s", header.c_str()))
	{
		if (lightCount == 0)
		{
			ImGui::TextDisabled("No directional lights in scene");
		}

		for (std::size_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
		{
			const std::string label = "Directional Light " + std::to_string(lightIndex + 1);
			if (m_filterText.empty() || Strings::ContainsIgnoreCase(label, m_filterText))
			{
				DrawSelectionEntry(label.c_str(), "L", "Directional Light", SceneObjectSelection::DirectionalLight(lightIndex));
			}
		}
		ImGui::TreePop();
	}
}

void SceneOutlinerPanel::BuildMeshSection() noexcept
{
	const std::size_t meshCount = m_gameScene->GetMeshes().GetMeshCount();
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	const std::string header = "Meshes (" + std::to_string(meshCount) + ")";
	if (ImGui::TreeNodeEx("Meshes", flags, "%s", header.c_str()))
	{
		if (meshCount == 0)
		{
			ImGui::TextDisabled("No meshes in scene");
		}

		for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			const std::string label = BuildMeshLabel(meshIndex);
			if (m_filterText.empty() || Strings::ContainsIgnoreCase(label, m_filterText))
			{
				DrawSelectionEntry(label.c_str(), "M", "Static Mesh", SceneObjectSelection::Mesh(meshIndex));
			}
		}
		ImGui::TreePop();
	}
}

void SceneOutlinerPanel::SyncVisibilityState() noexcept
{
	if (m_gameScene == nullptr)
	{
		return;
	}

	m_lightVisibility.resize(m_gameScene->GetLighting().GetDirectionalLightCount(), true);
	m_meshVisibility.resize(m_gameScene->GetMeshes().GetMeshCount(), true);
}

bool SceneOutlinerPanel::IsEntryVisible(const SceneObjectSelection& selection) const noexcept
{
	switch (selection.type)
	{
		case SceneObjectType::Camera:
			return m_cameraVisible;
		case SceneObjectType::DirectionalLight:
			return selection.index >= m_lightVisibility.size() || m_lightVisibility[selection.index];
		case SceneObjectType::Mesh:
			return selection.index >= m_meshVisibility.size() || m_meshVisibility[selection.index];
		case SceneObjectType::None:
		default:
			return true;
	}
}

void SceneOutlinerPanel::ToggleEntryVisibility(const SceneObjectSelection& selection) noexcept
{
	switch (selection.type)
	{
		case SceneObjectType::Camera:
			m_cameraVisible = !m_cameraVisible;
			break;
		case SceneObjectType::DirectionalLight:
			if (selection.index < m_lightVisibility.size())
			{
				m_lightVisibility[selection.index] = !m_lightVisibility[selection.index];
			}
			break;
		case SceneObjectType::Mesh:
			if (selection.index < m_meshVisibility.size())
			{
				m_meshVisibility[selection.index] = !m_meshVisibility[selection.index];
			}
			break;
		case SceneObjectType::None:
		default:
			break;
	}
}

void SceneOutlinerPanel::DrawSelectionEntry(
    const char* label,
    const char* iconText,
    const char* typeLabel,
    const SceneObjectSelection& selection) noexcept
{
	const bool isSelected = m_selection != nullptr && *m_selection == selection;
	const bool isVisible = IsEntryVisible(selection);
	ImGui::PushID(label);
	ImGui::Indent(12.0f);
	if (UiUtil::DrawVisibilityIconButton("visibility", isVisible))
	{
		ToggleEntryVisibility(selection);
	}
	ImGui::SameLine(0.0f, 4.0f);
	UiUtil::DrawPlaceholderTypeIcon(iconText, typeLabel);
	ImGui::SameLine(0.0f, 6.0f);
	if (!isVisible)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
	}
	if (ImGui::Selectable(label, isSelected, 0, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
	{
		*m_selection = selection;
	}
	if (!isVisible)
	{
		ImGui::PopStyleColor();
	}
	ImGui::Unindent(12.0f);
	ImGui::PopID();
}

void SceneOutlinerPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	constexpr float kMinWidth = 220.0f;
	const float kMaxWidth = (std::max) (kMinWidth + 1.0f, io.DisplaySize.x * 0.5f);
	m_widthPixels = std::clamp(m_widthPixels, kMinWidth, kMaxWidth);
	const float panelHeight = (std::max) (1.0f, io.DisplaySize.y - m_topInsetPixels);

	ImGui::SetNextWindowPos(ImVec2(0.0f, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, panelHeight), ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, panelHeight), ImVec2(kMaxWidth, panelHeight));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(
	    "Scene Outliner",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar();

	m_widthPixels = ImGui::GetWindowWidth();
	UiUtil::DrawPanelHeader("Scene", "Outliner");

	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		ImGui::TextDisabled("Scene outliner unavailable");
		ImGui::End();
		return;
	}

	EnsureValidSelection();
	SyncVisibilityState();
	ImGui::BeginDisabled(disableInteraction);
	BuildToolbar();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	BuildCameraSection();
	BuildLightSection();
	BuildMeshSection();
	ImGui::EndDisabled();

	ImGui::End();
}