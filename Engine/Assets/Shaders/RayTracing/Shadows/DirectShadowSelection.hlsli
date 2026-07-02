#ifndef SPARKLE_DIRECT_SHADOW_SELECTION_HLSLI
#define SPARKLE_DIRECT_SHADOW_SELECTION_HLSLI

#include "Lighting/LightSampling.hlsli"
#include "Resources/ConstantBuffers.hlsli"

namespace DirectShadowSelection
{
	struct SelectedLight
	{
		uint Type;
		uint Index;
	};

	SelectedLight SelectFirstShadowCastingLight()
	{
		[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.DirectionalLightCount; ++lightIndex)
		{
			if (DirectionalLights[lightIndex].CastShadow != 0u)
			{
				SelectedLight selectedLight;
				selectedLight.Type = LightSampling::LightTypeDirectional;
				selectedLight.Index = lightIndex;
				return selectedLight;
			}
		}

		[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.PointLightCount; ++lightIndex)
		{
			if (PointLights[lightIndex].CastShadow != 0u)
			{
				SelectedLight selectedLight;
				selectedLight.Type = LightSampling::LightTypePoint;
				selectedLight.Index = lightIndex;
				return selectedLight;
			}
		}

		[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.SpotLightCount; ++lightIndex)
		{
			if (SpotLights[lightIndex].CastShadow != 0u)
			{
				SelectedLight selectedLight;
				selectedLight.Type = LightSampling::LightTypeSpot;
				selectedLight.Index = lightIndex;
				return selectedLight;
			}
		}

		[loop] for (uint lightIndex = 0u; lightIndex < ViewLighting.RectLightCount; ++lightIndex)
		{
			if (RectLights[lightIndex].CastShadow != 0u)
			{
				SelectedLight selectedLight;
				selectedLight.Type = LightSampling::LightTypeRect;
				selectedLight.Index = lightIndex;
				return selectedLight;
			}
		}

		SelectedLight selectedLight;
		selectedLight.Type = LightSampling::LightTypeInvalid;
		selectedLight.Index = LightSampling::LightIndexInvalid;
		return selectedLight;
	}
}

#endif
