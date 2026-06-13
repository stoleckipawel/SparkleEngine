#include "PCH.h"

#include "RenderLightingBuilder.h"

#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "SceneData/RenderSceneData.h"

#include <cmath>

static const auto g_renderLightingBuilderLogger = Logging::GetOrCreateLogger("Renderer.SceneData");

namespace RenderLightingBuilder
{
	void Build(const LightingSnapshot& lightingSnapshot, RenderSceneData& sceneData) noexcept
	{
		sceneData.directionalLights.clear();
		sceneData.directionalLights.reserve(lightingSnapshot.directionalLights.size());
		sceneData.pointLights.clear();
		sceneData.pointLights.reserve(lightingSnapshot.pointLights.size());
		sceneData.spotLights.clear();
		sceneData.spotLights.reserve(lightingSnapshot.spotLights.size());

		for (const DirectionalLightDesc& light : lightingSnapshot.directionalLights)
		{
			DirectionalLight renderLight = {};
			renderLight.direction = light.direction;
			renderLight.intensity = light.intensity;
			renderLight.color = light.color;
			renderLight.angularDiameterRadians = light.angularDiameterRadians;
			renderLight.castShadow = light.castShadow;
			sceneData.directionalLights.push_back(renderLight);
		}

		for (const PointLightSnapshotDesc& light : lightingSnapshot.pointLights)
		{
			PointLight renderLight = {};
			renderLight.position = light.position;
			renderLight.range = light.range;
			renderLight.color = light.color;
			renderLight.intensity = light.intensity;
			renderLight.sourceRadius = light.sourceRadius;
			renderLight.castShadow = light.castShadow;
			sceneData.pointLights.push_back(renderLight);
		}

		for (const SpotLightSnapshotDesc& light : lightingSnapshot.spotLights)
		{
			SpotLight renderLight = {};
			renderLight.position = light.position;
			renderLight.range = light.range;
			renderLight.sourceRadius = light.sourceRadius;
			renderLight.direction = light.direction;
			renderLight.innerConeCosine = std::cos(light.innerConeAngleRadians);
			renderLight.color = light.color;
			renderLight.intensity = light.intensity;
			renderLight.outerConeCosine = std::cos(light.outerConeAngleRadians);
			renderLight.castShadow = light.castShadow;
			sceneData.spotLights.push_back(renderLight);
		}

		static bool loggedFirstLightingSummary = false;
		if (!loggedFirstLightingSummary &&
		    (!sceneData.directionalLights.empty() || !sceneData.pointLights.empty() || !sceneData.spotLights.empty()))
		{
			loggedFirstLightingSummary = true;
			SPDLOG_LOGGER_INFO(
			    g_renderLightingBuilderLogger,
			    "RenderLightingBuilder: prepared scene lighting (directionalLights={}, pointLights={}, spotLights={}).",
			    sceneData.directionalLights.size(),
			    sceneData.pointLights.size(),
			    sceneData.spotLights.size());
		}
	}
}
