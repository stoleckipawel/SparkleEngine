#include "PCH.h"

#include "Scene/Preparation/RenderLightPreparation.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/Lighting/LightRenderingControls.h"

#include <cmath>
#include <type_traits>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_renderLightPreparationLogger, "Renderer.RenderLightPreparation");

static DirectionalLight PrepareDirectional(const SceneLightDesc& light, const SceneDirectionalLightDesc& directional) noexcept
{
	return DirectionalLight{directional.direction, directional.illuminance, light.common.color, directional.angularSizeRadians, directional.castShadow};
}

static PointLight PreparePoint(const SceneLightDesc& light, const PointLightDesc& point, const DirectX::XMFLOAT3& position) noexcept
{
	return PointLight{
	    .position = position,
	    .range = point.range,
	    .color = light.common.color,
	    .luminousIntensity = point.luminousIntensity,
	    .radius = point.radius,
	    .distanceAttenuationCoefficients = point.distanceAttenuationCoefficients,
	    .castShadow = point.castShadow};
}

static SpotLight PrepareSpot(const SceneLightDesc& light, const SpotLightDesc& spot, const DirectX::XMFLOAT3& position) noexcept
{
	return SpotLight{
	    .position = position,
	    .range = spot.range,
	    .radius = spot.radius,
	    .direction = spot.direction,
	    .innerAngleCosine = std::cos(spot.innerAngleRadians),
	    .color = light.common.color,
	    .luminousIntensity = spot.luminousIntensity,
	    .outerAngleCosine = std::cos(spot.outerAngleRadians),
	    .distanceAttenuationCoefficients = spot.distanceAttenuationCoefficients,
	    .castShadow = spot.castShadow};
}

static RectLight PrepareRect(const SceneLightDesc& light, const RectLightDesc& rect, const DirectX::XMFLOAT3& position) noexcept
{
	return RectLight{
	    .position = position,
	    .width = rect.width,
	    .direction = rect.direction,
	    .height = rect.height,
	    .tangent = rect.tangent,
	    .luminance = rect.luminance,
	    .color = light.common.color,
	    .castShadow = rect.castShadow};
}

void PrepareRenderLights(std::span<const RenderLightData> inputs, std::span<PreparedRenderLight> outputs) noexcept
{
	if (inputs.size() != outputs.size())
	{
		Diagnostics::Fatal(g_renderLightPreparationLogger, __FILE__, __LINE__, "Render-light preparation input and output counts differ.");
	}

	for (std::size_t index = 0u; index < inputs.size(); ++index)
	{
		const RenderLightData& row = inputs[index];
		PreparedRenderLight& output = outputs[index];
		output = PreparedRenderLight{.Object = row.Object};

		const SceneLightDesc& light = row.Description;
		if (!light.common.visible || !IsLightRenderingEnabled(light.GetKind()))
		{
			continue;
		}

		const DirectX::XMFLOAT3 position{light.common.worldTransform._41, light.common.worldTransform._42, light.common.worldTransform._43};

		if (const SceneDirectionalLightDesc* directional = light.GetDirectional())
		{
			output.Payload = PrepareDirectional(light, *directional);
		}
		else if (const PointLightDesc* point = light.GetPoint())
		{
			output.Payload = PreparePoint(light, *point, position);
		}
		else if (const SpotLightDesc* spot = light.GetSpot())
		{
			output.Payload = PrepareSpot(light, *spot, position);
		}
		else if (const RectLightDesc* rect = light.GetRect())
		{
			output.Payload = PrepareRect(light, *rect, position);
		}
	}
}

void CommitPreparedRenderLights(std::span<const PreparedRenderLight> lights, PreparedRenderScene& preparedScene)
{
	for (const PreparedRenderLight& light : lights)
	{
		std::visit(
		    [&]<typename TLight>(const TLight& value)
		    {
			    if constexpr (std::is_same_v<TLight, DirectionalLight>)
			    {
				    preparedScene.directionalLights.Add(light.Object, value);
			    }
			    else if constexpr (std::is_same_v<TLight, PointLight>)
			    {
				    preparedScene.pointLights.Add(light.Object, value);
			    }
			    else if constexpr (std::is_same_v<TLight, SpotLight>)
			    {
				    preparedScene.spotLights.Add(light.Object, value);
			    }
			    else if constexpr (std::is_same_v<TLight, RectLight>)
			    {
				    preparedScene.rectLights.Add(light.Object, value);
			    }
		    },
		    light.Payload);
	}
}
