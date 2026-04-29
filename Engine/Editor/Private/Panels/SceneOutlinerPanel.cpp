#include "PCH.h"
#include "Panels/SceneOutlinerPanel.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Scene/SceneObjectSelection.h"
#include "Scene/Camera/CameraComponent.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/DirectionalLightComponent.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>

#include <imgui.h>

namespace
{
	UiUtil::EditorIcon BuildSelectionIcon(const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return UiUtil::EditorIcon::Camera;
			case SceneObjectType::DirectionalLight:
				return UiUtil::EditorIcon::DirectionalLight;
			case SceneObjectType::Mesh:
				return UiUtil::EditorIcon::StaticMesh;
			case SceneObjectType::None:
			default:
				return UiUtil::EditorIcon::None;
		}
	}

	bool DrawFilterChip(const char* label, bool active) noexcept
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 2.0f));
		if (active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, SparkleUiPalette::ButtonBackgroundActive());
			ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, SparkleUiPalette::ButtonBackground());
			ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
		}

		ImGui::PushID("FilterChip");
		const bool pressed = ImGui::SmallButton(label);
		ImGui::PopID();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		return pressed;
	}

	void DrawMutedText(const char* text, float alpha = 0.72f) noexcept
	{
		ImVec4 color = SparkleUiPalette::TextMuted();
		color.w *= alpha;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(text);
		ImGui::PopStyleColor();
	}

	bool DrawCenteredVisibilityButton(const char* id, bool visible) noexcept
	{
		constexpr float kVisibilityIconSize = 14.0f;
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float horizontalOffset = (std::max) (0.0f, (availableWidth - kVisibilityIconSize) * 0.5f);
		const float verticalOffset = (std::max) (0.0f, (ImGui::GetFrameHeight() - kVisibilityIconSize) * 0.5f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalOffset);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);
		return UiUtil::DrawVisibilityIconButton(id, visible);
	}
}  // namespace

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
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
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

	ImGui::Spacing();
	if (DrawFilterChip("All", m_activeFilter == SceneOutlinerFilter::All))
	{
		m_activeFilter = SceneOutlinerFilter::All;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (DrawFilterChip("Camera", m_activeFilter == SceneOutlinerFilter::Cameras))
	{
		m_activeFilter = SceneOutlinerFilter::Cameras;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (DrawFilterChip("Light", m_activeFilter == SceneOutlinerFilter::Lights))
	{
		m_activeFilter = SceneOutlinerFilter::Lights;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (DrawFilterChip("Mesh", m_activeFilter == SceneOutlinerFilter::Meshes))
	{
		m_activeFilter = SceneOutlinerFilter::Meshes;
	}
	ImGui::PopStyleVar();
}

void SceneOutlinerPanel::BuildFooter() noexcept
{
	const std::size_t totalCount = 1 + m_gameScene->GetLighting().GetDirectionalLightCount() + m_gameScene->GetMeshes().GetMeshCount();
	const std::size_t displayedCount = CountVisibleEntries();
	const bool hasValidSelection = IsSelectionValid();
	ImGui::Separator();
	ImGui::TextDisabled("%zu actors%s", displayedCount, hasValidSelection ? " (1 selected)" : "");
	if (displayedCount != totalCount)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("of %zu", totalCount);
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
	if (m_activeFilter != SceneOutlinerFilter::All && m_activeFilter != SceneOutlinerFilter::Cameras)
	{
		return;
	}

	bool open = true;
	DrawSectionRow("Camera", "Camera", 1, open);
	if (open)
	{
		const SceneObjectSelection selection = SceneObjectSelection::Camera();
		if (PassesActiveFilter(selection) && MatchesSearch("Scene Camera", "Camera"))
		{
			DrawSelectionEntry("Scene Camera", "Camera", selection);
		}
	}
}

void SceneOutlinerPanel::BuildLightSection() noexcept
{
	if (m_activeFilter != SceneOutlinerFilter::All && m_activeFilter != SceneOutlinerFilter::Lights)
	{
		return;
	}

	const std::size_t lightCount = m_gameScene->GetLighting().GetDirectionalLightCount();
	bool open = true;
	DrawSectionRow("Lights", "Lights", lightCount, open);
	if (open)
	{
		if (lightCount == 0)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);
			ImGui::TextDisabled("No directional lights in scene");
		}

		for (std::size_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
		{
			const std::string label = "Directional Light " + std::to_string(lightIndex + 1);
			const SceneObjectSelection selection = SceneObjectSelection::DirectionalLight(lightIndex);
			if (PassesActiveFilter(selection) && MatchesSearch(label.c_str(), "Directional Light"))
			{
				DrawSelectionEntry(label.c_str(), "Directional Light", selection);
			}
		}
	}
}

void SceneOutlinerPanel::BuildMeshSection() noexcept
{
	if (m_activeFilter != SceneOutlinerFilter::All && m_activeFilter != SceneOutlinerFilter::Meshes)
	{
		return;
	}

	const std::size_t meshCount = m_gameScene->GetMeshes().GetMeshCount();
	bool open = true;
	DrawSectionRow("Meshes", "Meshes", meshCount, open);
	if (open)
	{
		if (meshCount == 0)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);
			ImGui::TextDisabled("No meshes in scene");
		}

		for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			const std::string label = BuildMeshLabel(meshIndex);
			const SceneObjectSelection selection = SceneObjectSelection::Mesh(meshIndex);
			if (PassesActiveFilter(selection) && MatchesSearch(label.c_str(), "Static Mesh"))
			{
				DrawSelectionEntry(label.c_str(), "Static Mesh", selection);
			}
		}
	}
}

bool SceneOutlinerPanel::PassesActiveFilter(const SceneObjectSelection& selection) const noexcept
{
	switch (m_activeFilter)
	{
		case SceneOutlinerFilter::Cameras:
			return selection.type == SceneObjectType::Camera;
		case SceneOutlinerFilter::Lights:
			return selection.type == SceneObjectType::DirectionalLight;
		case SceneOutlinerFilter::Meshes:
			return selection.type == SceneObjectType::Mesh;
		case SceneOutlinerFilter::All:
		default:
			return true;
	}
}

bool SceneOutlinerPanel::MatchesSearch(const char* label, const char* typeLabel) const noexcept
{
	return m_filterText.empty() || Strings::ContainsIgnoreCase(label, m_filterText) || Strings::ContainsIgnoreCase(typeLabel, m_filterText);
}

std::size_t SceneOutlinerPanel::CountVisibleEntries() const noexcept
{
	std::size_t count = 0;
	const SceneObjectSelection cameraSelection = SceneObjectSelection::Camera();
	if (PassesActiveFilter(cameraSelection) && MatchesSearch("Scene Camera", "Camera"))
	{
		++count;
	}

	const std::size_t lightCount = m_gameScene->GetLighting().GetDirectionalLightCount();
	for (std::size_t lightIndex = 0; lightIndex < lightCount; ++lightIndex)
	{
		const std::string label = "Directional Light " + std::to_string(lightIndex + 1);
		const SceneObjectSelection selection = SceneObjectSelection::DirectionalLight(lightIndex);
		if (PassesActiveFilter(selection) && MatchesSearch(label.c_str(), "Directional Light"))
		{
			++count;
		}
	}

	const std::size_t meshCount = m_gameScene->GetMeshes().GetMeshCount();
	for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
	{
		const std::string label = BuildMeshLabel(meshIndex);
		const SceneObjectSelection selection = SceneObjectSelection::Mesh(meshIndex);
		if (PassesActiveFilter(selection) && MatchesSearch(label.c_str(), "Static Mesh"))
		{
			++count;
		}
	}

	return count;
}

bool SceneOutlinerPanel::IsEntryVisible(const SceneObjectSelection& selection) const noexcept
{
	switch (selection.type)
	{
		case SceneObjectType::Camera:
			return m_gameScene->GetSceneCamera().GetCameraComponent().IsVisible();
		case SceneObjectType::DirectionalLight:
			return selection.index >= m_gameScene->GetLighting().GetDirectionalLightCount() ||
			       m_gameScene->GetLighting().GetDirectionalLightComponent(selection.index).IsVisible();
		case SceneObjectType::Mesh:
			if (selection.index >= m_gameScene->GetMeshes().GetMeshCount())
			{
				return true;
			}
			if (const MeshComponent* meshComponent = m_gameScene->GetMeshes().GetMeshComponent(selection.index))
			{
				return meshComponent->IsVisible();
			}
			return true;
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
		{
			CameraComponent& camera = m_gameScene->GetSceneCamera().GetCameraComponent();
			camera.SetVisible(!camera.IsVisible());
			break;
		}
		case SceneObjectType::DirectionalLight:
			if (selection.index < m_gameScene->GetLighting().GetDirectionalLightCount())
			{
				DirectionalLightComponent& light = m_gameScene->GetLighting().GetDirectionalLightComponent(selection.index);
				light.SetVisible(!light.IsVisible());
			}
			break;
		case SceneObjectType::Mesh:
			if (selection.index < m_gameScene->GetMeshes().GetMeshCount())
			{
				if (MeshComponent* meshComponent = m_gameScene->GetMeshes().GetMeshComponent(selection.index))
				{
					meshComponent->SetVisible(!meshComponent->IsVisible());
				}
			}
			break;
		case SceneObjectType::None:
		default:
			break;
	}
}

void SceneOutlinerPanel::DrawSectionRow(const char* id, const char* label, std::size_t count, bool& open) noexcept
{
	ImGui::TableNextRow();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, SparkleUiPalette::SectionHeaderBackground());
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(id);
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	open = ImGui::TreeNodeEx("##section", flags, "+ %s (%zu)", label, count);
	ImGui::PopID();
	ImGui::TableSetColumnIndex(2);
	DrawMutedText("Folder", 0.58f);
	if (open)
	{
		ImGui::TreePop();
	}
}

void SceneOutlinerPanel::DrawSelectionEntry(const char* label, const char* typeLabel, const SceneObjectSelection& selection) noexcept
{
	const bool isSelected = m_selection != nullptr && *m_selection == selection;
	const bool isVisible = IsEntryVisible(selection);
	ImGui::TableNextRow();
	if (isSelected)
	{
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::SelectionOverlay()));
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::SelectionOverlay()));
	}
	ImGui::PushID(label);
	ImGui::TableSetColumnIndex(0);
	if (DrawCenteredVisibilityButton("visibility", isVisible))
	{
		ToggleEntryVisibility(selection);
	}

	ImGui::TableSetColumnIndex(1);
	ImGui::Indent(16.0f);
	UiUtil::DrawEditorIcon(BuildSelectionIcon(selection), typeLabel);
	ImGui::SameLine(0.0f, 6.0f);
	if (!isVisible)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
	}
	else if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
	}
	if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
	{
		*m_selection = selection;
	}
	if (!isVisible || isSelected)
	{
		ImGui::PopStyleColor();
	}
	ImGui::Unindent(16.0f);
	ImGui::TableSetColumnIndex(2);
	DrawMutedText(typeLabel, 0.62f);
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
	ImGui::BeginDisabled(disableInteraction);
	BuildToolbar();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 2.0f));
	if (ImGui::BeginTable(
	        "##OutlinerTable",
	        3,
	        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
	            ImGuiTableFlags_NoPadOuterX,
	        ImVec2(0.0f, -28.0f)))
	{
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
		ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 88.0f);
		ImGui::TableHeadersRow();

		BuildCameraSection();
		BuildLightSection();
		BuildMeshSection();
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	BuildFooter();
	ImGui::EndDisabled();

	ImGui::End();
}