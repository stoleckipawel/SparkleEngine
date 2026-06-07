#include "PCH.h"
#include "Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.h"

#include "Config/RenderConfig.h"
#include "Core/Public/Math/MathUtils.h"

#include <algorithm>
#include <cstdint>

namespace
{
	constexpr std::size_t kMaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;
	constexpr std::size_t kMaxPointLights = RenderConfig::Lights::MaxPointLights;
	constexpr std::size_t kMaxSpotLights = RenderConfig::Lights::MaxSpotLights;

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
		return desc;
	}

	SpotLightSnapshotDesc BuildSpotLightDesc(const SceneLightCommonDesc& common, const SpotLightDesc& spot) noexcept
	{
		SpotLightSnapshotDesc desc;
		desc.position = ExtractWorldPosition(common.worldTransform);
		desc.range = (std::max) (0.0f, spot.range);
		desc.direction = MathUtils::Normalize3(spot.direction, {0.0f, -1.0f, 0.0f});
		desc.innerConeAngleRadians = (std::max) (0.0f, spot.innerConeAngleRadians);
		desc.color = common.color;
		desc.intensity = (std::max) (0.0f, common.intensity);
		desc.outerConeAngleRadians = (std::max) (desc.innerConeAngleRadians, spot.outerConeAngleRadians);
		return desc;
	}

	template <typename AppendLight>
	void AppendDirectionalLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		std::size_t appendedCount = 0;
		for (const SceneLightDesc& light : lights)
		{
			if (appendedCount >= kMaxDirectionalLights)
			{
				break;
			}

			const SceneDirectionalLightDesc* directional = light.GetDirectional();
			if (directional == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildDirectionalLightDesc(light.common, *directional), appendedCount);
			++appendedCount;
		}
	}

	template <typename AppendLight>
	void AppendPointLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		std::size_t appendedCount = 0;
		for (const SceneLightDesc& light : lights)
		{
			if (appendedCount >= kMaxPointLights)
			{
				break;
			}

			const PointLightDesc* point = light.GetPoint();
			if (point == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildPointLightDesc(light.common, *point), appendedCount);
			++appendedCount;
		}
	}

	template <typename AppendLight>
	void AppendSpotLights(const std::vector<SceneLightDesc>& lights, bool requireVisible, AppendLight appendLight) noexcept
	{
		std::size_t appendedCount = 0;
		for (const SceneLightDesc& light : lights)
		{
			if (appendedCount >= kMaxSpotLights)
			{
				break;
			}

			const SpotLightDesc* spot = light.GetSpot();
			if (spot == nullptr || (requireVisible && !light.common.visible))
			{
				continue;
			}

			appendLight(BuildSpotLightDesc(light.common, *spot), appendedCount);
			++appendedCount;
		}
	}
}  // namespace

namespace SceneLightingSnapshotBuilder
{
	LevelLightingDesc BuildLevelDesc(const std::vector<SceneLightDesc>& lights) noexcept
	{
		LevelLightingDesc desc = {};
		AppendDirectionalLights(
		    lights,
		    false,
		    [&desc](const DirectionalLightDesc& light, std::size_t index) noexcept
		    {
			    desc.directionalLights[index] = light;
			    desc.directionalLightCount = static_cast<std::uint32_t>(index + 1);
		    });
		return desc;
	}

	LightingSnapshot BuildSnapshot(const std::vector<SceneLightDesc>& lights) noexcept
	{
		LightingSnapshot snapshot = {};
		AppendDirectionalLights(
		    lights,
		    true,
		    [&snapshot](const DirectionalLightDesc& light, std::size_t index) noexcept
		    {
			    snapshot.directionalLights[index] = light;
			    snapshot.directionalLightCount = static_cast<std::uint32_t>(index + 1);
		    });
		AppendPointLights(
		    lights,
		    true,
		    [&snapshot](const PointLightSnapshotDesc& light, std::size_t index) noexcept
		    {
			    snapshot.pointLights[index] = light;
			    snapshot.pointLightCount = static_cast<std::uint32_t>(index + 1);
		    });
		AppendSpotLights(
		    lights,
		    true,
		    [&snapshot](const SpotLightSnapshotDesc& light, std::size_t index) noexcept
		    {
			    snapshot.spotLights[index] = light;
			    snapshot.spotLightCount = static_cast<std::uint32_t>(index + 1);
		    });
		return snapshot;
	}
}  // namespace SceneLightingSnapshotBuilder
