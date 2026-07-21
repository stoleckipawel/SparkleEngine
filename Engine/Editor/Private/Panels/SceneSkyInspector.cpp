#include "PCH.h"
#include "Panels/SceneSkyInspector.h"

#include "Scene/Transactions/EditorTransactionManager.h"
#include "Util/UiUtil.h"
#include "World/SkyEnvironment.h"

#include <algorithm>

void SceneSkyInspector::Build(const std::optional<SkyEnvironment>& current, EditorTransactionManager& transactions,
                              std::uint64_t generation, const std::string& filter) noexcept
{
	bool hasSky = current.has_value();
	if (UiUtil::MatchesDetailsFilter(filter, "Sky", "override default texture color intensity enabled visible") &&
	    UiUtil::BeginDetailsCategory("Sky"))
	{
		const bool defaultHasSky = false;
		if (UiUtil::EditDetailsCheckbox("Override Default", hasSky, &defaultHasSky))
		{
			std::optional<SkyEnvironment> after = hasSky ? std::optional<SkyEnvironment>{SkyEnvironment{}} : std::nullopt;
			(void) transactions.Execute({0, SetSkyEnvironmentCommand{after}}, {0, SetSkyEnvironmentCommand{current}}, generation);
		}
		if (!hasSky) UiUtil::DrawDetailsValueRow("Source", "Engine Default");
		UiUtil::EndDetailsCategory();
	}
	if (!hasSky || !current) return;

	SceneSkyDesc after = current->Description;
	bool changed = false;
	if (UiUtil::MatchesDetailsFilter(filter, "Rendering", "sky texture color intensity enabled visible") &&
	    UiUtil::BeginDetailsCategory("Rendering"))
	{
		const bool defaultEnabled = true;
		changed |= UiUtil::EditDetailsCheckbox("Enabled", after.enabled, &defaultEnabled);
		float color[3] = {after.color.x, after.color.y, after.color.z}; const float defaultColor[3] = {1.0f, 1.0f, 1.0f};
		if (UiUtil::EditDetailsColor3("Color", color, defaultColor))
		{
			after.color = {(std::max)(0.0f, color[0]), (std::max)(0.0f, color[1]), (std::max)(0.0f, color[2])}; changed = true;
		}
		const float defaultIntensity = 1.0f;
		if (UiUtil::EditDetailsFloat("Intensity", after.intensity, 0.01f, 0.0f, 0.0f, "%.3f", &defaultIntensity))
		{
			after.intensity = (std::max)(0.0f, after.intensity); changed = true;
		}
		const std::string defaultTexture;
		if (UiUtil::EditDetailsText("Texture", after.skyTexture.texturePath, &defaultTexture))
		{
			after.skyTexture.textureGroup = TextureGroup::HdrColor; changed = true;
		}
		UiUtil::EndDetailsCategory();
	}
	if (changed)
		(void) transactions.Execute({0, SetSkyEnvironmentCommand{SkyEnvironment{after}}},
		                           {0, SetSkyEnvironmentCommand{current}}, generation, "sky-environment");
}
