#include "PCH.h"

#include "Scene/Preparation/RenderLightPreparation.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include <cmath>

static const auto g_renderLightPreparationLogger = Logging::GetOrCreateLogger("Renderer.RenderLightPreparation");

void RenderLightPreparation::PrepareRange(std::span<const RenderLightData> inputs, std::span<PreparedRenderLight> outputs) noexcept
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
		if (!light.common.visible)
		{
			continue;
		}

		const DirectX::XMFLOAT3 position{light.common.worldTransform._41, light.common.worldTransform._42, light.common.worldTransform._43};

		if (const SceneDirectionalLightDesc* directional = light.GetDirectional())
		{
			PrepareDirectional(light, *directional, output);
		}
		else if (const PointLightDesc* point = light.GetPoint())
		{
			PreparePoint(light, *point, position, output);
		}
		else if (const SpotLightDesc* spot = light.GetSpot())
		{
			PrepareSpot(light, *spot, position, output);
		}
		else if (const RectLightDesc* rect = light.GetRect())
		{
			PrepareRect(light, *rect, position, output);
		}
	}
}

void RenderLightPreparation::PrepareDirectional(
    const SceneLightDesc& light,
    const SceneDirectionalLightDesc& directional,
    PreparedRenderLight& output) noexcept
{
	output.Classification = RenderLightClassification::Directional;
	output.Directional = DirectionalLight{
	    directional.direction,
	    directional.illuminance,
	    light.common.color,
	    directional.angularSizeRadians,
	    directional.castShadow};
}

void RenderLightPreparation::PreparePoint(
    const SceneLightDesc& light,
    const PointLightDesc& point,
    const DirectX::XMFLOAT3& position,
    PreparedRenderLight& output) noexcept
{
	output.Classification = RenderLightClassification::Point;
	output.Point = PointLight{
	    .position = position,
	    .range = point.range,
	    .color = light.common.color,
	    .luminousIntensity = point.luminousIntensity,
	    .radius = point.radius,
	    .distanceAttenuationCoefficients = point.distanceAttenuationCoefficients,
	    .castShadow = point.castShadow};
}

void RenderLightPreparation::PrepareSpot(
    const SceneLightDesc& light,
    const SpotLightDesc& spot,
    const DirectX::XMFLOAT3& position,
    PreparedRenderLight& output) noexcept
{
	output.Classification = RenderLightClassification::Spot;
	output.Spot = SpotLight{
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

void RenderLightPreparation::PrepareRect(
    const SceneLightDesc& light,
    const RectLightDesc& rect,
    const DirectX::XMFLOAT3& position,
    PreparedRenderLight& output) noexcept
{
	output.Classification = RenderLightClassification::Rect;
	output.Rect = RectLight{
	    .position = position,
	    .width = rect.width,
	    .direction = rect.direction,
	    .height = rect.height,
	    .tangent = rect.tangent,
	    .luminance = rect.luminance,
	    .color = light.common.color,
	    .castShadow = rect.castShadow};
}

void RenderLightPreparation::Commit(std::span<const PreparedRenderLight> lights, PreparedRenderScene& preparedScene)
{
	for (const PreparedRenderLight& light : lights)
	{
		switch (light.Classification)
		{
			case RenderLightClassification::Directional:
				preparedScene.directionalLights.Add(light.Object, light.Directional);
				break;
			case RenderLightClassification::Point:
				preparedScene.pointLights.Add(light.Object, light.Point);
				break;
			case RenderLightClassification::Spot:
				preparedScene.spotLights.Add(light.Object, light.Spot);
				break;
			case RenderLightClassification::Rect:
				preparedScene.rectLights.Add(light.Object, light.Rect);
				break;
			case RenderLightClassification::None:
				break;
		}
	}
}
