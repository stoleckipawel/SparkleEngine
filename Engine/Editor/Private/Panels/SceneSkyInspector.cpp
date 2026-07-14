#include "PCH.h"
#include "Panels/SceneSkyInspector.h"

#include "Scene/GameScene.h"
#include "Scene/Sky/SceneSky.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <utility>

void SceneSkyInspector::Build(GameScene& gameScene, const std::string& filterText) noexcept
{
	SceneSky& sceneSky = gameScene.GetSky();
	bool hasSky = sceneSky.HasSky();
	if (UiUtil::MatchesDetailsFilter(filterText, "Sky", "override default texture color intensity enabled visible"))
	{
		if (UiUtil::BeginDetailsCategory("Sky"))
		{
			constexpr bool kDefaultHasSky = false;
			if (UiUtil::EditDetailsCheckbox("Override Default", hasSky, &kDefaultHasSky))
			{
				if (hasSky)
				{
					sceneSky.SetSky();
				}
				else
				{
					sceneSky.RemoveSky();
				}
			}

			if (!hasSky)
			{
				UiUtil::DrawDetailsValueRow("Source", "Engine Default");
			}
			UiUtil::EndDetailsCategory();
		}
	}

	const SceneSkyDesc* currentSky = sceneSky.GetSky();
	if (!hasSky || currentSky == nullptr)
	{
		return;
	}

	SceneSkyDesc sky = *currentSky;
	bool skyChanged = false;
	if (UiUtil::MatchesDetailsFilter(filterText, "Rendering", "sky texture color intensity enabled visible"))
	{
		if (UiUtil::BeginDetailsCategory("Rendering"))
		{
			constexpr bool kDefaultEnabled = true;
			skyChanged |= UiUtil::EditDetailsCheckbox("Enabled", sky.enabled, &kDefaultEnabled);

			float color[3] = {sky.color.x, sky.color.y, sky.color.z};
			const float defaultColor[3] = {1.0f, 1.0f, 1.0f};
			if (UiUtil::EditDetailsColor3("Color", color, defaultColor))
			{
				sky.color = {(std::max) (0.0f, color[0]), (std::max) (0.0f, color[1]), (std::max) (0.0f, color[2])};
				skyChanged = true;
			}

			constexpr float kDefaultIntensity = 1.0f;
			if (UiUtil::EditDetailsFloat("Intensity", sky.intensity, 0.01f, 0.0f, 0.0f, "%.3f", &kDefaultIntensity))
			{
				sky.intensity = (std::max) (0.0f, sky.intensity);
				skyChanged = true;
			}

			const std::string defaultTexture;
			if (UiUtil::EditDetailsText("Texture", sky.skyTexture.texturePath, &defaultTexture))
			{
				sky.skyTexture.textureGroup = TextureGroup::HdrColor;
				skyChanged = true;
			}
			UiUtil::EndDetailsCategory();
		}
	}

	if (skyChanged)
	{
		sceneSky.SetSky(std::move(sky));
	}
}
