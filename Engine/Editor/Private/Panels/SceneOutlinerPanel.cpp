#include "PCH.h"
#include "Panels/SceneOutlinerPanel.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Panels/SceneOutlinerEntries.h"
#include "Scene/SceneObjectSelection.h"
#include "Scene/SceneObjectActions.h"
#include "Scene/SceneObjectPresentation.h"
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
	const std::string searchHint = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Search actors, lights, meshes...");
	if (ImGui::InputTextWithHint("##SceneFilter", searchHint.c_str(), filterBuffer, sizeof(filterBuffer)))
	{
		m_filterText = filterBuffer;
	}

	ImGui::Spacing();
	if (UiUtil::DrawFilterChip("All", m_activeFilter == SceneOutlinerFilter::All))
	{
		m_activeFilter = SceneOutlinerFilter::All;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (UiUtil::DrawFilterChip("Camera", m_activeFilter == SceneOutlinerFilter::Cameras))
	{
		m_activeFilter = SceneOutlinerFilter::Cameras;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (UiUtil::DrawFilterChip("Light", m_activeFilter == SceneOutlinerFilter::Lights))
	{
		m_activeFilter = SceneOutlinerFilter::Lights;
	}
	ImGui::SameLine(0.0f, 4.0f);
	if (UiUtil::DrawFilterChip("Mesh", m_activeFilter == SceneOutlinerFilter::Meshes))
	{
		m_activeFilter = SceneOutlinerFilter::Meshes;
	}
	ImGui::PopStyleVar();
}

void SceneOutlinerPanel::BuildFooter() noexcept
{
	const std::size_t totalCount =
	    m_gameScene->GetCameras().GetCameraCount() + m_gameScene->GetLighting().GetLightCount() + m_gameScene->GetMeshes().GetMeshCount();
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

	return SceneObjectActions::IsSelectionValid(*m_gameScene, *m_selection);
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

	const std::vector<SceneOutlinerEntry> entries = SceneOutlinerEntries::BuildCameraEntries(*m_gameScene);
	DrawEntrySection("Camera", "Camera", "No cameras in scene", entries);
}

void SceneOutlinerPanel::BuildLightSection() noexcept
{
	if (m_activeFilter != SceneOutlinerFilter::All && m_activeFilter != SceneOutlinerFilter::Lights)
	{
		return;
	}

	const std::vector<SceneOutlinerEntry> entries = SceneOutlinerEntries::BuildLightEntries(*m_gameScene);
	DrawEntrySection("Lights", "Lights", "No lights in scene", entries);
}

void SceneOutlinerPanel::BuildMeshSection() noexcept
{
	if (m_activeFilter != SceneOutlinerFilter::All && m_activeFilter != SceneOutlinerFilter::Meshes)
	{
		return;
	}

	const std::vector<SceneOutlinerEntry> entries = SceneOutlinerEntries::BuildMeshEntries(*m_gameScene);
	DrawEntrySection("Meshes", "Meshes", "No meshes in scene", entries);
}

void SceneOutlinerPanel::DrawEntrySection(
    const char* id,
    const char* label,
    const char* emptyText,
    const std::vector<SceneOutlinerEntry>& entries) noexcept
{
	bool open = true;
	DrawSectionRow(id, label, entries.size(), open);
	if (open)
	{
		if (entries.empty())
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);
			ImGui::TextDisabled("%s", emptyText);
		}

		for (const SceneOutlinerEntry& entry : entries)
		{
			if (PassesActiveFilter(entry.selection) && MatchesSearch(entry.label.c_str(), entry.typeLabel.c_str()))
			{
				DrawSelectionEntry(entry.label.c_str(), entry.typeLabel.c_str(), entry.selection);
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
			return selection.type == SceneObjectType::Light;
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
	const auto countVisible = [this](const std::vector<SceneOutlinerEntry>& entries) noexcept
	{
		std::size_t visibleCount = 0;
		for (const SceneOutlinerEntry& entry : entries)
		{
			if (PassesActiveFilter(entry.selection) && MatchesSearch(entry.label.c_str(), entry.typeLabel.c_str()))
			{
				++visibleCount;
			}
		}
		return visibleCount;
	};

	count += countVisible(SceneOutlinerEntries::BuildCameraEntries(*m_gameScene));
	count += countVisible(SceneOutlinerEntries::BuildLightEntries(*m_gameScene));
	count += countVisible(SceneOutlinerEntries::BuildMeshEntries(*m_gameScene));

	return count;
}

bool SceneOutlinerPanel::IsEntryVisible(const SceneObjectSelection& selection) const noexcept
{
	return SceneObjectActions::IsVisible(*m_gameScene, selection);
}

void SceneOutlinerPanel::ToggleEntryVisibility(const SceneObjectSelection& selection) noexcept
{
	SceneObjectActions::ToggleVisibility(*m_gameScene, selection);
}

void SceneOutlinerPanel::SelectEntry(const SceneObjectSelection& selection) noexcept
{
	if (m_selection == nullptr || m_gameScene == nullptr)
	{
		return;
	}

	*m_selection = selection;
	SceneObjectActions::ApplySelection(*m_gameScene, selection);
}

void SceneOutlinerPanel::DrawSectionRow(const char* id, const char* label, std::size_t count, bool& open) noexcept
{
	ImGui::TableNextRow();
	ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, SparkleUiPalette::SectionHeaderBackground());
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(id);
	const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	const std::string sectionLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::FolderOpen, label);
	open = ImGui::TreeNodeEx("##section", flags, "%s (%zu)", sectionLabel.c_str(), count);
	ImGui::PopID();
	ImGui::TableSetColumnIndex(2);
	const std::string typeLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Folder, "Folder");
	UiUtil::DrawMutedText(typeLabel.c_str(), 0.58f);
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
	if (UiUtil::DrawCenteredVisibilityIconButton("visibility", isVisible))
	{
		ToggleEntryVisibility(selection);
	}

	ImGui::TableSetColumnIndex(1);
	ImGui::Indent(16.0f);
	UiUtil::DrawEditorIcon(SceneObjectPresentation::BuildSelectionIcon(selection, m_gameScene), typeLabel, !isSelected);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		SelectEntry(selection);
	}
	ImGui::SameLine(0.0f, 6.0f);
	if (!isVisible)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
	}
	else if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
	}
	if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	}
	if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
	{
		SelectEntry(selection);
	}
	if (isSelected)
	{
		ImGui::PopStyleColor(2);
	}
	if (!isVisible || isSelected)
	{
		ImGui::PopStyleColor();
	}
	ImGui::Unindent(16.0f);
	ImGui::TableSetColumnIndex(2);
	ImVec4 mutedTypeColor = SparkleUiPalette::TextMuted();
	mutedTypeColor.w *= 0.62f;
	ImGui::PushStyleColor(ImGuiCol_Text, mutedTypeColor);
	if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	}
	if (ImGui::Selectable(typeLabel, false, 0, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
	{
		SelectEntry(selection);
	}
	if (isSelected)
	{
		ImGui::PopStyleColor(2);
	}
	ImGui::PopStyleColor();
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
