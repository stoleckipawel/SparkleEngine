#include "PCH.h"
#include "Scene/Lighting/SceneLighting.h"

#include "World/GameWorld.h"
#include "World/GameWorldState.h"

SceneLighting::SceneLighting(GameWorld& world) noexcept : m_world(&world) {}

std::size_t SceneLighting::GetLightCount() const noexcept { return m_world->m_state->GetLightCount(); }

EntityId SceneLighting::GetLightEntity(std::size_t index) const noexcept { return m_world->m_state->GetLightEntity(index); }

std::optional<SceneLightDesc> SceneLighting::GetLight(std::size_t index) const { return GetLight(GetLightEntity(index)); }

std::optional<SceneLightDesc> SceneLighting::GetLight(EntityId entity) const { return m_world->m_state->ReadLight(entity); }

bool SceneLighting::IsLightVisible(std::size_t index) const noexcept { return IsLightVisible(GetLightEntity(index)); }

bool SceneLighting::IsLightVisible(EntityId entity) const noexcept { return m_world->m_state->ReadVisibility(entity); }

void SceneLighting::SetLightVisible(std::size_t index, bool visible) { SetLightVisible(GetLightEntity(index), visible); }

void SceneLighting::SetLightVisible(EntityId entity, bool visible) { m_world->m_state->WriteVisibility(entity, visible); }

bool SceneLighting::SetLight(std::size_t lightIndex, SceneLightDesc light)
{
	return SetLight(GetLightEntity(lightIndex), std::move(light));
}

bool SceneLighting::SetLight(EntityId entity, SceneLightDesc light)
{
	return m_world->m_state->WriteLight(entity, std::move(light));
}

void SceneLighting::ApplyFromDesc(const std::vector<SceneLightDesc>& lights)
{
	for (const SceneLightDesc& light : lights)
	{
		m_world->m_state->AddLight(SceneLightDesc(light));
	}
}

void SceneLighting::AppendLight(SceneLightDesc light) { m_world->m_state->AddLight(std::move(light)); }

std::vector<SceneLightDesc> SceneLighting::CaptureToDesc() const { return m_world->m_state->CaptureLightsToDesc(); }

LightingSnapshot SceneLighting::CaptureSnapshot() const { return m_world->m_state->CaptureLighting(); }
