#include "PCH.h"
#include "Scene/Sky/SceneSky.h"

#include "World/GameWorld.h"
#include "World/GameWorldState.h"

#include <utility>

void SceneSky::ApplyFromDesc(std::optional<SceneSkyDesc> sky) noexcept
{
	if (sky)
	{
		m_world->m_state->WriteSkyEnvironment(SkyEnvironment{.Description = std::move(*sky)});
	}
	else
	{
		m_world->m_state->RemoveSkyEnvironment();
	}
}

void SceneSky::Reset() noexcept
{
	m_world->m_state->RemoveSkyEnvironment();
}

bool SceneSky::HasSky() const noexcept
{
	return m_world->m_state->HasSkyEnvironment();
}

std::optional<SceneSkyDesc> SceneSky::GetSky() const
{
	const std::optional<SkyEnvironment> sky = m_world->m_state->ReadSkyEnvironment();
	return sky ? std::optional<SceneSkyDesc>(sky->Description) : std::nullopt;
}

void SceneSky::SetSky(SceneSkyDesc sky)
{
	m_world->m_state->WriteSkyEnvironment(SkyEnvironment{.Description = std::move(sky)});
}

void SceneSky::RemoveSky() noexcept
{
	m_world->m_state->RemoveSkyEnvironment();
}

std::optional<SceneSkyDesc> SceneSky::CaptureToDesc() const
{
	return GetSky();
}

SceneSkySnapshot SceneSky::CaptureSnapshot() const
{
	return SceneSkySnapshot{GetSky()};
}
