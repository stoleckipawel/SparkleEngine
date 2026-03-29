#include "PCH.h"
#include "Scene/Lighting/SceneLighting.h"

void SceneLighting::ApplyFromDesc(const LevelLightingDesc& desc) noexcept
{
	m_gameDirectionalLights.clear();
	const std::size_t count = (std::min) (static_cast<std::size_t>(desc.directionalLightCount), MaxDirectionalLights);
	m_gameDirectionalLights.reserve(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		m_gameDirectionalLights.emplace_back(desc.directionalLights[i]);
	}
}

LevelLightingDesc SceneLighting::CaptureToDesc() const noexcept
{
	LevelLightingDesc desc = {};
	desc.directionalLightCount = static_cast<std::uint32_t>((std::min) (m_gameDirectionalLights.size(), MaxDirectionalLights));
	for (std::size_t i = 0; i < desc.directionalLightCount; ++i)
	{
		desc.directionalLights[i] = m_gameDirectionalLights[i].GetDesc();
	}
	return desc;
}

LightingSnapshot SceneLighting::CaptureSnapshot() const noexcept
{
	LightingSnapshot snapshot = {};
	snapshot.directionalLightCount = static_cast<std::uint32_t>((std::min) (m_gameDirectionalLights.size(), MaxDirectionalLights));
	for (std::size_t i = 0; i < snapshot.directionalLightCount; ++i)
	{
		snapshot.directionalLights[i] = m_gameDirectionalLights[i].GetDesc();
	}
	return snapshot;
}
