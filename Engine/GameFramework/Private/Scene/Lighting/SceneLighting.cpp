#include "PCH.h"
#include "Scene/Lighting/SceneLighting.h"

#include "Scene/GameScene.h"
#include "World/SceneWorld.h"

SceneLighting::SceneLighting(GameScene& scene) noexcept : m_scene(&scene) {}

std::size_t SceneLighting::GetLightCount() const noexcept { return m_scene->m_world->GetLightCount(); }

EntityId SceneLighting::GetLightEntity(std::size_t index) const noexcept { return m_scene->m_world->GetLightEntity(index); }

std::optional<SceneLightDesc> SceneLighting::GetLight(std::size_t index) const { return GetLight(GetLightEntity(index)); }

std::optional<SceneLightDesc> SceneLighting::GetLight(EntityId entity) const { return m_scene->m_world->ReadLight(entity); }

bool SceneLighting::IsLightVisible(std::size_t index) const noexcept { return IsLightVisible(GetLightEntity(index)); }

bool SceneLighting::IsLightVisible(EntityId entity) const noexcept { return m_scene->m_world->ReadVisibility(entity); }

void SceneLighting::SetLightVisible(std::size_t index, bool visible) { SetLightVisible(GetLightEntity(index), visible); }

void SceneLighting::SetLightVisible(EntityId entity, bool visible) { m_scene->m_world->WriteVisibility(entity, visible); }

bool SceneLighting::SetLight(std::size_t lightIndex, SceneLightDesc light)
{
	return SetLight(GetLightEntity(lightIndex), std::move(light));
}

bool SceneLighting::SetLight(EntityId entity, SceneLightDesc light)
{
	return m_scene->m_world->WriteLight(entity, std::move(light));
}

void SceneLighting::ApplyFromDesc(const std::vector<SceneLightDesc>& lights)
{
	for (const SceneLightDesc& light : lights)
	{
		m_scene->m_world->AddLight(SceneLightDesc(light));
	}
}

void SceneLighting::AppendLight(SceneLightDesc light) { m_scene->m_world->AddLight(std::move(light)); }

std::vector<SceneLightDesc> SceneLighting::CaptureToDesc() const { return m_scene->m_world->CaptureLightsToDesc(); }

LightingSnapshot SceneLighting::CaptureSnapshot() const noexcept { return m_scene->m_world->CaptureLighting(); }
