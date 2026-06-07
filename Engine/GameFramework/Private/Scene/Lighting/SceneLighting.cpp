#include "PCH.h"
#include "Scene/Lighting/SceneLighting.h"

#include "Scene/Lighting/Loading/LevelLightingSceneBuilder.h"
#include "Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.h"

#include <utility>

void SceneLighting::ApplyFromDesc(const LevelLightingDesc& desc)
{
	m_lights = LevelLightingSceneBuilder::BuildLights(desc);
}

void SceneLighting::AppendLight(SceneLightDesc light)
{
	m_lights.push_back(std::move(light));
}

const SceneLightDesc* SceneLighting::GetLight(std::size_t index) const noexcept
{
	return index < m_lights.size() ? &m_lights[index] : nullptr;
}

bool SceneLighting::IsLightVisible(std::size_t index) const noexcept
{
	const SceneLightDesc* light = GetLight(index);
	return light == nullptr || light->common.visible;
}

void SceneLighting::SetLightVisible(std::size_t index, bool visible)
{
	if (index >= m_lights.size())
	{
		return;
	}

	m_lights[index].common.visible = visible;
}

bool SceneLighting::ApplyLightDesc(std::size_t lightIndex, SceneLightDesc light)
{
	if (lightIndex >= m_lights.size())
	{
		return false;
	}

	m_lights[lightIndex] = std::move(light);
	return true;
}

LevelLightingDesc SceneLighting::CaptureToDesc() const noexcept
{
	return SceneLightingSnapshotBuilder::BuildLevelDesc(m_lights);
}

LightingSnapshot SceneLighting::CaptureSnapshot() const noexcept
{
	return SceneLightingSnapshotBuilder::BuildSnapshot(m_lights);
}

void SceneLighting::Reset() noexcept
{
	m_lights.clear();
}
