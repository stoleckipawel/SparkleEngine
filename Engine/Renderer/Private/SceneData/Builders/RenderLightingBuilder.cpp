#include "PCH.h"
#include "RenderLightingBuilder.h"

#include "Core/Public/Math/MathUtils.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <cmath>

namespace RenderLightingBuilder
{
	void Build(const std::vector<RenderLightData>& lights, RenderSceneData& sceneData) noexcept
	{
		sceneData.directionalLights.Clear();
		sceneData.pointLights.Clear();
		sceneData.spotLights.Clear();
		sceneData.rectLights.Clear();
		for (const RenderLightData& row : lights)
		{
			const SceneLightDesc& light = row.Description;
			if (!light.common.visible) continue;
			const DirectX::XMFLOAT3 position{light.common.worldTransform._41, light.common.worldTransform._42,
			                                 light.common.worldTransform._43};
			if (const SceneDirectionalLightDesc* value = light.GetDirectional())
			{
				sceneData.directionalLights.Add(
				    row.Object,
				    {MathUtils::Normalize3(value->direction, {0.0f, -1.0f, 0.0f}),
				     (std::max)(0.0f, light.common.intensity), light.common.color,
				     (std::max)(0.0f, value->angularDiameterRadians), value->castShadow});
			}
			else if (const PointLightDesc* value = light.GetPoint())
			{
				sceneData.pointLights.Add(row.Object, {position, (std::max)(0.0f, value->range), light.common.color,
				                                       (std::max)(0.0f, light.common.intensity),
				                                       (std::max)(0.0f, value->sourceRadius), value->castShadow});
			}
			else if (const SpotLightDesc* value = light.GetSpot())
			{
				const float inner = (std::max)(0.0f, value->innerConeAngleRadians);
				const float outer = (std::max)(inner, value->outerConeAngleRadians);
				sceneData.spotLights.Add(row.Object, {position, (std::max)(0.0f, value->range),
				                                      (std::max)(0.0f, value->sourceRadius),
				                                      MathUtils::Normalize3(value->direction, {0.0f, -1.0f, 0.0f}),
				                                      std::cos(inner), light.common.color,
				                                      (std::max)(0.0f, light.common.intensity), std::cos(outer),
				                                      value->castShadow});
			}
			else if (const RectLightDesc* value = light.GetRect())
			{
				sceneData.rectLights.Add(row.Object, {position, (std::max)(0.0f, value->width),
				                                      MathUtils::Normalize3(value->direction, {0.0f, -1.0f, 0.0f}),
				                                      (std::max)(0.0f, value->height),
				                                      MathUtils::Normalize3(value->tangent, {1.0f, 0.0f, 0.0f}),
				                                      (std::max)(0.0f, light.common.intensity), light.common.color,
				                                      value->castShadow});
			}
		}
	}
}
