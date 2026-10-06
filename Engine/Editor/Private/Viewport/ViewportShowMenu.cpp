#include "PCH.h"
#include "Viewport/ViewportShowMenu.h"
#include "Util/UiUtil.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <array>
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
	const std::string menuLabel = UiUtil::MakeIconLabel(icon, label);
	if (!ImGui::BeginMenu(menuLabel.c_str()))
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
	ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
	if (ImGui::MenuItem("All", anyEnabled && !allEnabled ? "Mixed" : nullptr, allEnabled))
	{
		SetLightingShowIntent(executor, leaves, !anyEnabled, error);
	}
	ImGui::Separator();
	for (std::size_t index = 0; index < leaves.size(); ++index)
	{
		const std::string leafLabel = UiUtil::MakeIconLabel(leaves[index].Icon, leaves[index].Label);
		if (ImGui::MenuItem(leafLabel.c_str(), nullptr, intent[index]))
		{
			SetLightingShowIntent(executor, leaves.subspan(index, 1), !intent[index], error);
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s\nAcknowledged intent; shared by all applicable viewports.", leaves[index].CVarName);
		}
	}
	ImGui::PopItemFlag();
	ImGui::EndMenu();
}

void DrawViewportShowMenu(const CVarControlExecutor* executor, bool disableInteraction, std::string& error)
{
	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button("Show"))
	{
		ImGui::OpenPopup("ViewportShowMenu");
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Shared lighting controls for all applicable viewports; not viewport-local overrides.");
	}
	ImGui::SetNextWindowSizeConstraints(ImVec2(260.0f, 0.0f), ImVec2(420.0f, 600.0f));
	if (!ImGui::BeginPopup("ViewportShowMenu"))
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
		ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
		const std::string defaultsLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Reset, "Use Defaults");
		if (ImGui::MenuItem(defaultsLabel.c_str()))
		{
			SetLightingShowIntent(*executor, lightingShowLeaves, true, error);
		}
		ImGui::PopItemFlag();
		ImGui::SeparatorText("LIGHTING COMPONENTS");
		DrawLightingShowGroup(
		    "Direct Lighting",
		    UiUtil::EditorIcon::DirectionalLight,
		    std::span(lightingShowLeaves).subspan(0, 3),
		    std::span(intent).subspan(0, 3),
		    *executor,
		    error);
		DrawLightingShowGroup(
		    "Indirect Lighting",
		    UiUtil::EditorIcon::Light,
		    std::span(lightingShowLeaves).subspan(3, 2),
		    std::span(intent).subspan(3, 2),
		    *executor,
		    error);
		ImGui::SeparatorText("LIGHTING FEATURES");
		DrawLightingShowGroup(
		    "Shadows",
		    UiUtil::EditorIcon::ViewAmbientOcclusion,
		    std::span(lightingShowLeaves).subspan(5, 2),
		    std::span(intent).subspan(5, 2),
		    *executor,
		    error);
		ImGui::EndDisabled();
	}
	if (!error.empty())
	{
		ImGui::Separator();
		ImGui::TextWrapped("Lighting edit rejected: %s", error.c_str());
	}
	ImGui::EndPopup();
}
