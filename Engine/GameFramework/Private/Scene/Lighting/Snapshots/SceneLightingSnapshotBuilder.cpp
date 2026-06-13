#include "PCH.h"
#include "Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.h"

#include "Core/Public/Math/MathUtils.h"

#include <algorithm>

namespace
{
	DirectX::XMFLOAT3 ExtractWorldPosition(const DirectX::XMFLOAT4X4& transform) noexcept
	{
		return {transform._41, transform._42, transform._43};
	}

	DirectionalLightDesc BuildDirectionalLightDesc(
	    const SceneLightCommonDesc& common,
	    const SceneDirectionalLightDesc& directional) noexcept
	{
		DirectionalLightDesc desc;
		desc.direction = MathUtils::Normalize3(directional.direction, {0.0f, -1.0f, 0.0f});
		desc.intensity = (std::max) (0.0f, common.intensity);
		desc.color = common.color;
		desc.angularDiameterRadians = (std::max) (0.0f, directional.angularDiameterRadians);
		desc.castShadow = directional.castShadow;
		return desc;
	}

	PointLightSnapshotDesc BuildPointLightDesc(const SceneLightCommonDesc& common, const PointLightDesc& point) noexcept
	{
		PointLightSnapshotDesc desc;
		desc.position = ExtractWorldPosition(common.worldTransform);
		desc.range = (std::max) (0.0f, point.range);
		desc.color = common.color;
		desc.intensity = (std::max) (0.0f, common.intensity);
		desc.sourceRadius = (std::max) (0.0f, point.sourceRadius);
		desc.castShadow = point.castShadow;
		return desc;
	}

	SpotLightSnapshotDesc BuildSpotLightDesc(const SceneLightCommonDesc& common, const SpotLightDesc& spot) noexcept
	{
		SpotLightSnapshotDesc desc;
		desc.position = ExtractWorldPosition(common.worldTransform);
		desc.range = (std::max) (0.0f, spot.range);
		desc.sourceRadius = (std::max) (0.0f, spot.sourceRadius);
		desc.direction = MathUtils::Normalize3(spot.direction, {0.0f, -1.0f, 0.0f});
		desc.innerConeAngleRadians = (std::max) (0.0f, spot.innerConeAngleRadians);
		desc.color = common.color;
		desc.intensity = (std::max) (0.0f, common.intensity);
		desc.outerConeAngleRadians = (std::max) (desc.innerConeAngleRadians, spot.outerConeAngleRadians);
		desc.castShadow = spot.castShadow;
		return desc;
	}

	template <typename AppendLight>
	void AppendDirectionalLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		for (const SceneLightDesc& light : lights)
		{
			const SceneDirectionalLightDesc* directional = light.GetDirectional();
			if (directional == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildDirectionalLightDesc(light.common, *directional));
		}
	}

	template <typename AppendLight>
	void AppendPointLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		for (const SceneLightDesc& light : lights)
		{
			const PointLightDesc* point = light.GetPoint();
			if (point == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildPointLightDesc(light.common, *point));
		}
	}

	template <typename AppendLight>
	void AppendSpotLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		for (const SceneLightDesc& light : lights)
		{
			const SpotLightDesc* spot = light.GetSpot();
			if (spot == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildSpotLightDesc(light.common, *spot));
		}
	}
}  // namespace

namespace SceneLightingSnapshotBuilder
{
	LightingSnapshot BuildSnapshot(const std::vector<SceneLightDesc>& lights) noexcept
	{
		LightingSnapshot snapshot = {};
		AppendDirectionalLights(
		    lights,
		    true,
		    [&snapshot](const DirectionalLightDesc& light) noexcept
		    {
			    snapshot.directionalLights.push_back(light);
		    });
		AppendPointLights(
		    lights,
		    true,
		    [&snapshot](const PointLightSnapshotDesc& light) noexcept
		    {
			    snapshot.pointLights.push_back(light);
		    });
		AppendSpotLights(
		    lights,
		    true,
		    [&snapshot](const SpotLightSnapshotDesc& light) noexcept
		    {
			    snapshot.spotLights.push_back(light);
		    });
		return snapshot;
	}
}  // namespace SceneLightingSnapshotBuilder
