#include "PCH.h"
#include "Viewport/ViewportShowMenu.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <array>
#include <algorithm>
#include <span>
#include <utility>

struct LightingShowLeaf final
{
	const char* Label;
	const char* CVarName;
	UiUtil::EditorIcon Icon;
};

static constexpr std::array<LightingShowLeaf, 7> lightingShowLeaves = {
    {{"Diffuse", "r.Lighting.Direct.Diffuse", UiUtil::EditorIcon::ViewDirectDiffuse},
        {"Specular", "r.Lighting.Direct.Specular", UiUtil::EditorIcon::ViewDirectSpecular},
        {"Subsurface", "r.Lighting.Direct.Subsurface", UiUtil::EditorIcon::ViewDirectSubsurface},
        {"Diffuse", "r.Lighting.Indirect.Diffuse", UiUtil::EditorIcon::ViewDirectDiffuse},
        {"Specular", "r.Lighting.Indirect.Specular", UiUtil::EditorIcon::ViewDirectSpecular},
        {"Direct Shadows", "r.Lighting.Shadows.Direct", UiUtil::EditorIcon::ViewAmbientOcclusion},
        {"Indirect Shadows", "r.Lighting.Shadows.Indirect", UiUtil::EditorIcon::ViewAmbientOcclusion}}};

struct LightingShowGroup final
{
	const char* Label;
	UiUtil::EditorIcon Icon;
	std::size_t FirstLeaf;
	std::size_t LeafCount;
	const char* Section;
};

static constexpr auto lightingShowGroups = std::to_array<LightingShowGroup>(
    {{"Direct Lighting", UiUtil::EditorIcon::DirectionalLight, 0, 3, "LIGHTING COMPONENTS"},
        {"Indirect Lighting", UiUtil::EditorIcon::Light, 3, 2, nullptr},
        {"Shadows", UiUtil::EditorIcon::ViewAmbientOcclusion, 5, 2, "LIGHTING FEATURES"}});

static bool QueryLightingShowIntent(
    const CVarControlExecutor& executor,
    std::array<bool, lightingShowLeaves.size()>& intent,
    std::string& error)
{
	CVarControlRequest request;
	request.Entries.reserve(lightingShowLeaves.size());
	for (const LightingShowLeaf& leaf : lightingShowLeaves)
	{
		request.Entries.push_back({leaf.CVarName, {}});
	}
	const CVarControlResult result = executor(std::move(request));
	if (!result.Error.empty())
	{
		error = result.Error;
		return false;
	}
	if (result.Values.size() != lightingShowLeaves.size())
	{
		error = "Lighting control query returned an incomplete response.";
		return false;
	}
	for (std::size_t index = 0; index < lightingShowLeaves.size(); ++index)
	{
		const CVarControlValue& value = result.Values[index];
		if (value.Name != lightingShowLeaves[index].CVarName || value.Type != "bool" || (value.Value != "true" && value.Value != "false"))
		{
			error = "Invalid lighting control response: " + std::string(lightingShowLeaves[index].CVarName);
			return false;
		}
		intent[index] = value.Value == "true";
	}
	return true;
}

static void SetLightingShowIntent(
    const CVarControlExecutor& executor,
    std::span<const LightingShowLeaf> leaves,
    bool enabled,
    std::string& error)
{
	CVarControlRequest request;
	request.Operation = CVarControlOperation::Set;
	request.Entries.reserve(leaves.size());
	for (const LightingShowLeaf& leaf : leaves)
	{
		request.Entries.push_back({leaf.CVarName, enabled ? "true" : "false"});
	}
	error = executor(std::move(request)).Error;
}

static void DrawLightingShowGroup(
    const char* label,
    UiUtil::EditorIcon icon,
    std::span<const LightingShowLeaf> leaves,
    std::span<const bool> intent,
    const CVarControlExecutor& executor,
    std::string& error)
{
	float menuWidth = UiUtil::MeasureMenuRow("All", UiUtil::MenuRowKind::Toggle);
	for (const LightingShowLeaf& leaf : leaves)
	{
		menuWidth = (std::max) (menuWidth, UiUtil::MeasureMenuRow(leaf.Label, UiUtil::MenuRowKind::Toggle));
	}
	if (!UiUtil::BeginMenu(label, icon, menuWidth))
	{
		return;
	}
	bool anyEnabled = false;
	bool allEnabled = true;
	for (bool enabled : intent)
	{
		anyEnabled |= enabled;
		allEnabled &= enabled;
	}
	const UiUtil::MenuCheckState groupState = allEnabled ? UiUtil::MenuCheckState::Checked
	    : anyEnabled                                     ? UiUtil::MenuCheckState::Mixed
	                                                     : UiUtil::MenuCheckState::Unchecked;
	if (UiUtil::DrawMenuItem("All", UiUtil::EditorIcon::None, groupState, ImGuiSelectableFlags_NoAutoClosePopups))
	{
		SetLightingShowIntent(executor, leaves, !anyEnabled, error);
	}
	if (anyEnabled && !allEnabled && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Mixed selection. Toggle to disable all %s controls.", label);
	}
	ImGui::Separator();
	for (std::size_t index = 0; index < leaves.size(); ++index)
	{
		const UiUtil::MenuCheckState leafState = intent[index] ? UiUtil::MenuCheckState::Checked : UiUtil::MenuCheckState::Unchecked;
		if (UiUtil::DrawMenuItem(leaves[index].Label, leaves[index].Icon, leafState, ImGuiSelectableFlags_NoAutoClosePopups))
		{
			SetLightingShowIntent(executor, leaves.subspan(index, 1), !intent[index], error);
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s\nAcknowledged intent; shared by all applicable viewports.", leaves[index].CVarName);
		}
	}
	ImGui::EndMenu();
}

void DrawViewportShowMenu(const CVarControlExecutor* executor, bool disableInteraction, std::string& error)
{
	ImGui::BeginDisabled(disableInteraction);
	const std::string showLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::EyeVisible, "Show");
	if (ImGui::Button(showLabel.c_str()))
	{
		ImGui::OpenPopup("ViewportShowMenu");
	}
	ImGui::EndDisabled();
	const ImVec2 popupPosition(ImGui::GetItemRectMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowHeight());
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Shared lighting controls for all applicable viewports; not viewport-local overrides.");
	}
	if (!ImGui::IsPopupOpen("ViewportShowMenu"))
	{
		return;
	}
	const UiUtil::MenuStyleScope menuStyle;
	float menuWidth = UiUtil::MeasureMenuRow("Use Defaults", UiUtil::MenuRowKind::Action);
	for (const LightingShowGroup& group : lightingShowGroups)
	{
		menuWidth = (std::max) (menuWidth, UiUtil::MeasureMenuRow(group.Label, UiUtil::MenuRowKind::Submenu));
		if (group.Section != nullptr)
		{
			menuWidth = (std::max) (menuWidth, UiUtil::MeasureMenuSection(group.Section));
		}
	}
	if (!UiUtil::BeginMenuPopup("ViewportShowMenu", popupPosition, menuWidth))
	{
		return;
	}
	std::array<bool, lightingShowLeaves.size()> intent{};
	std::string queryError;
	const bool available = executor != nullptr && *executor && QueryLightingShowIntent(*executor, intent, queryError);
	if (!available)
	{
		ImGui::TextWrapped(
		    "Lighting controls unavailable: %s",
		    queryError.empty() ? "Host control executor is missing." : queryError.c_str());
	}
	else
	{
		ImGui::BeginDisabled(disableInteraction);
		if (UiUtil::DrawMenuItem(
		        "Use Defaults",
		        UiUtil::EditorIcon::None,
		        UiUtil::MenuCheckState::Hidden,
		        ImGuiSelectableFlags_NoAutoClosePopups))
		{
			SetLightingShowIntent(*executor, lightingShowLeaves, true, error);
		}
		for (const LightingShowGroup& group : lightingShowGroups)
		{
			if (group.Section != nullptr)
			{
				UiUtil::DrawMenuSection(group.Section);
			}
			DrawLightingShowGroup(
			    group.Label,
			    group.Icon,
			    std::span(lightingShowLeaves).subspan(group.FirstLeaf, group.LeafCount),
			    std::span(intent).subspan(group.FirstLeaf, group.LeafCount),
			    *executor,
			    error);
		}
		ImGui::EndDisabled();
	}
	if (!error.empty())
	{
		ImGui::Separator();
		ImGui::TextWrapped("Lighting edit rejected: %s", error.c_str());
	}
	ImGui::EndPopup();
}
