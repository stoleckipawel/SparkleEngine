#include "PCH.h"

#include "SceneData/Preparation/RenderLightPreparation.h"

#include "Core/Public/Math/MathUtils.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <cmath>

void RenderLightPreparation::PrepareRange(
    std::span<const RenderLightData> inputs,
    std::span<PreparedRenderLight> outputs) noexcept
{
	const std::size_t count =
	    (std::min)(inputs.size(), outputs.size());
	for (std::size_t index = 0u; index < count; ++index)
	{
		const RenderLightData& row = inputs[index];
		PreparedRenderLight& output = outputs[index];
		output = PreparedRenderLight{.Object = row.Object};

		const SceneLightDesc& light = row.Description;
		if (!light.common.visible)
		{
			continue;
		}

		const DirectX::XMFLOAT3 position{
		    light.common.worldTransform._41,
		    light.common.worldTransform._42,
		    light.common.worldTransform._43};

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
	    MathUtils::Normalize3(directional.direction, {0.0f, -1.0f, 0.0f}),
	    (std::max)(0.0f, light.common.intensity),
	    light.common.color,
	    (std::max)(0.0f, directional.angularDiameterRadians),
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
	    position,
	    (std::max)(0.0f, point.range),
	    light.common.color,
	    (std::max)(0.0f, light.common.intensity),
	    (std::max)(0.0f, point.sourceRadius),
	    point.castShadow};
}

void RenderLightPreparation::PrepareSpot(
    const SceneLightDesc& light,
    const SpotLightDesc& spot,
    const DirectX::XMFLOAT3& position,
    PreparedRenderLight& output) noexcept
{
	const float inner = (std::max)(0.0f, spot.innerConeAngleRadians);
	const float outer = (std::max)(inner, spot.outerConeAngleRadians);

	output.Classification = RenderLightClassification::Spot;
	output.Spot = SpotLight{
	    position,
	    (std::max)(0.0f, spot.range),
	    (std::max)(0.0f, spot.sourceRadius),
	    MathUtils::Normalize3(spot.direction, {0.0f, -1.0f, 0.0f}),
	    std::cos(inner),
	    light.common.color,
	    (std::max)(0.0f, light.common.intensity),
	    std::cos(outer),
	    spot.castShadow};
}

void RenderLightPreparation::PrepareRect(
    const SceneLightDesc& light,
    const RectLightDesc& rect,
    const DirectX::XMFLOAT3& position,
    PreparedRenderLight& output) noexcept
{
	output.Classification = RenderLightClassification::Rect;
	output.Rect = RectLight{
	    position,
	    (std::max)(0.0f, rect.width),
	    MathUtils::Normalize3(rect.direction, {0.0f, -1.0f, 0.0f}),
	    (std::max)(0.0f, rect.height),
	    MathUtils::Normalize3(rect.tangent, {1.0f, 0.0f, 0.0f}),
	    (std::max)(0.0f, light.common.intensity),
	    light.common.color,
	    rect.castShadow};
}

void RenderLightPreparation::Commit(
    std::span<const PreparedRenderLight> lights,
    RenderSceneData& sceneData)
{
	for (const PreparedRenderLight& light : lights)
	{
		switch (light.Classification)
		{
			case RenderLightClassification::Directional:
				sceneData.directionalLights.Add(
				    light.Object,
				    light.Directional);
				break;
			case RenderLightClassification::Point:
				sceneData.pointLights.Add(
				    light.Object,
				    light.Point);
				break;
			case RenderLightClassification::Spot:
				sceneData.spotLights.Add(
				    light.Object,
				    light.Spot);
				break;
			case RenderLightClassification::Rect:
				sceneData.rectLights.Add(
				    light.Object,
				    light.Rect);
				break;
			case RenderLightClassification::None:
				break;
		}
	}
}
