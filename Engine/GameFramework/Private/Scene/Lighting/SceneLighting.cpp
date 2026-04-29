#include "PCH.h"
#include "Scene/Lighting/SceneLighting.h"

void SceneLighting::ApplyFromDesc(const LevelLightingDesc& desc) noexcept
{
	m_directionalLightComponents.clear();
	const std::size_t count = (std::min) (static_cast<std::size_t>(desc.directionalLightCount), MaxDirectionalLights);
	m_directionalLightComponents.reserve(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		m_directionalLightComponents.emplace_back(desc.directionalLights[i]);
	}
}

LevelLightingDesc SceneLighting::CaptureToDesc() const noexcept
{
	LevelLightingDesc desc = {};
	desc.directionalLightCount = static_cast<std::uint32_t>((std::min) (m_directionalLightComponents.size(), MaxDirectionalLights));
	for (std::size_t i = 0; i < desc.directionalLightCount; ++i)
	{
		desc.directionalLights[i] = m_directionalLightComponents[i].GetDesc();
	}
	return desc;
}

LightingSnapshot SceneLighting::CaptureSnapshot() const noexcept
{
	LightingSnapshot snapshot = {};
	for (const DirectionalLightComponent& lightComponent : m_directionalLightComponents)
	{
		if (!lightComponent.IsVisible())
		{
			continue;
		}

		if (snapshot.directionalLightCount >= MaxDirectionalLights)
		{
			break;
		}

		snapshot.directionalLights[snapshot.directionalLightCount] = lightComponent.GetDesc();
		++snapshot.directionalLightCount;
	}
	return snapshot;
}
