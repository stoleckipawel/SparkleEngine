#include "PCH.h"
#include "Panels/SceneOutlinerPanel.h"

#include "Scene/SceneObjectSelection.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <cctype>
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

bool SceneOutlinerPanel::MatchesFilter(const std::string& filter, const char* label)
{
	if (filter.empty() || label == nullptr)
	{
		return true;
	}

	std::string normalizedLabel(label);
	std::transform(normalizedLabel.begin(), normalizedLabel.end(), normalizedLabel.begin(), [](unsigned char value)
	{
		return static_cast<char>(std::tolower(value));
	});

	std::string normalizedFilter = filter;
	std::transform(normalizedFilter.begin(), normalizedFilter.end(), normalizedFilter.begin(), [](unsigned char value)
	{
		return static_cast<char>(std::tolower(value));
	});

	return normalizedLabel.find(normalizedFilter) != std::string::npos;
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
	const std::size_t copyLength = (std::min)(m_filterText.size(), sizeof(filterBuffer) - 1);
	if (copyLength > 0)
	{
		std::copy_n(m_filterText.data(), copyLength, filterBuffer);
	}

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##SceneFilter", "Search actors, lights, meshes...", filterBuffer, sizeof(filterBuffer)))
	{
		m_filterText = filterBuffer;
	}

	ImGui::Spacing();
	const std::string objectCount = std::to_string(1 + m_gameScene->GetLighting().GetDirectionalLightCount() + m_gameScene->GetMeshes().GetMeshCount());
	UiUtil::DrawKeyValueRow("Objects", objectCount.c_str());
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
		if (MatchesFilter(m_filterText, "Scene Camera"))
		{
			DrawSelectionEntry("Scene Camera", "CAM", SceneObjectSelection::Camera());
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
			if (MatchesFilter(m_filterText, label.c_str()))
			{
				DrawSelectionEntry(label.c_str(), "LGT", SceneObjectSelection::DirectionalLight(lightIndex));
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
			if (MatchesFilter(m_filterText, label.c_str()))
			{
				DrawSelectionEntry(label.c_str(), "SM", SceneObjectSelection::Mesh(meshIndex));
			}
		}
		ImGui::TreePop();
	}
}

void SceneOutlinerPanel::DrawSelectionEntry(const char* label, const char* typeLabel, const SceneObjectSelection& selection) noexcept
{
	const bool isSelected = m_selection != nullptr && *m_selection == selection;
	ImGui::PushID(label);
	ImGui::Indent(12.0f);
	if (ImGui::Selectable(label, isSelected, 0, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
	{
		*m_selection = selection;
	}
	if (typeLabel != nullptr)
	{
		const ImVec2 rectMin = ImGui::GetItemRectMin();
		const ImVec2 rectMax = ImGui::GetItemRectMax();
		const float badgeWidth = 32.0f;
		const ImVec2 badgeMin(rectMax.x - badgeWidth - 8.0f, rectMin.y + 3.0f);
		const ImVec2 badgeMax(rectMax.x - 8.0f, rectMax.y - 3.0f);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(58, 64, 74, 220), 3.0f);
		drawList->AddText(ImVec2(badgeMin.x + 6.0f, badgeMin.y + 2.0f), IM_COL32(220, 224, 230, 255), typeLabel);
	}
	ImGui::Unindent(12.0f);
	ImGui::PopID();
}

void SceneOutlinerPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0.0f, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, io.DisplaySize.y - m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.98f);

	ImGui::Begin(
	    "Scene Outliner",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	UiUtil::DrawPanelHeader("Scene", "Outliner");

	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		ImGui::TextDisabled("Scene outliner unavailable");
		ImGui::End();
		return;
	}

	EnsureValidSelection();
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