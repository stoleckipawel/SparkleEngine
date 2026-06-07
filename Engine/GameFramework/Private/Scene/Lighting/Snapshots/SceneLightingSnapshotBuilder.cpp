#include "PCH.h"
#include "Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.h"

#include "Config/RenderConfig.h"
#include "Core/Public/Math/MathUtils.h"

#include <algorithm>
#include <cstdint>

namespace
{
	constexpr std::size_t kMaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;

	DirectionalLightDesc BuildDirectionalLightDesc(
	    const SceneLightCommonDesc& common,
	    const SceneDirectionalLightDesc& directional) noexcept
	{
		DirectionalLightDesc desc;
		desc.direction = MathUtils::Normalize3(directional.direction, {0.0f, -1.0f, 0.0f});
		desc.intensity = (std::max) (0.0f, common.intensity);
		desc.color.x = std::clamp(common.color.x, 0.0f, 1.0f);
		desc.color.y = std::clamp(common.color.y, 0.0f, 1.0f);
		desc.color.z = std::clamp(common.color.z, 0.0f, 1.0f);
		desc.castShadow = directional.castShadow;
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
		return snapshot;
	}
}  // namespace SceneLightingSnapshotBuilder
