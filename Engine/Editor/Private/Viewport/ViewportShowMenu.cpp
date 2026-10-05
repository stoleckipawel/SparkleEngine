#include "PCH.h"
#include "Viewport/ViewportShowMenu.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <array>
#include <span>
#include <utility>

struct LightingShowLeaf final
{
	const char* Label;
	const char* CVarName;
};

static constexpr std::array<LightingShowLeaf, 7> lightingShowLeaves = {
    {{"Diffuse", "r.Lighting.Direct.Diffuse"},
        {"Specular", "r.Lighting.Direct.Specular"},
        {"Subsurface", "r.Lighting.Direct.Subsurface"},
        {"Diffuse", "r.Lighting.Indirect.Diffuse"},
        {"Specular", "r.Lighting.Indirect.Specular"},
        {"Direct Shadows", "r.Lighting.Shadows.Direct"},
        {"Indirect Shadows", "r.Lighting.Shadows.Indirect"}}};

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
    std::span<const LightingShowLeaf> leaves,
    std::span<const bool> intent,
    const CVarControlExecutor& executor,
    std::string& error)
{
	bool anyEnabled = false;
	bool allEnabled = true;
	for (bool enabled : intent)
	{
		anyEnabled |= enabled;
		allEnabled &= enabled;
	}
	ImGui::PushID(label);
	ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, anyEnabled && !allEnabled);
	bool checked = allEnabled;
	if (ImGui::Checkbox(label, &checked))
	{
		SetLightingShowIntent(executor, leaves, !anyEnabled, error);
	}
	ImGui::PopItemFlag();
	if (anyEnabled && !allEnabled)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(mixed)");
	}
	ImGui::Indent();
	for (std::size_t index = 0; index < leaves.size(); ++index)
	{
		bool enabled = intent[index];
		if (ImGui::Checkbox(leaves[index].Label, &enabled))
		{
			SetLightingShowIntent(executor, leaves.subspan(index, 1), enabled, error);
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s\nAcknowledged intent; shared by all applicable viewports.", leaves[index].CVarName);
		}
	}
	ImGui::Unindent();
	ImGui::PopID();
}

void DrawViewportShowMenu(const CVarControlExecutor* executor, RenderViewMode viewMode, bool disableInteraction, std::string& error)
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
	if (!ImGui::BeginPopup("ViewportShowMenu"))
	{
		return;
	}
	ImGui::TextDisabled("Lighting (shared feature intent)");
	ImGui::TextDisabled("Acknowledged controls; displayed frames may lag.");
	ImGui::Separator();
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
		const bool applicable = viewMode == RenderViewMode::Lit || viewMode == RenderViewMode::Wireframe
		    || (viewMode >= RenderViewMode::DirectDiffuse && viewMode <= RenderViewMode::IndirectSpecular);
		if (!applicable)
		{
			ImGui::TextWrapped(
			    "These controls apply to Lit and Lit-shaded Wireframe, not this inspection or Reference mode. Intent is retained.");
		}
		ImGui::BeginDisabled(disableInteraction || !applicable);
		DrawLightingShowGroup(
		    "Direct Lighting",
		    std::span(lightingShowLeaves).subspan(0, 3),
		    std::span(intent).subspan(0, 3),
		    *executor,
		    error);
		DrawLightingShowGroup(
		    "Indirect Lighting",
		    std::span(lightingShowLeaves).subspan(3, 2),
		    std::span(intent).subspan(3, 2),
		    *executor,
		    error);
		DrawLightingShowGroup("Shadows", std::span(lightingShowLeaves).subspan(5, 2), std::span(intent).subspan(5, 2), *executor, error);
		ImGui::Separator();
		if (ImGui::Button("Reset Lighting Features"))
		{
			SetLightingShowIntent(*executor, lightingShowLeaves, true, error);
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
