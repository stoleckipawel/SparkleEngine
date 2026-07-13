#include "PCH.h"
#include "Scene/Sky/SceneSky.h"

#include <utility>

void SceneSky::ApplyFromDesc(std::optional<SceneSkyDesc> sky) noexcept
{
	m_sky = std::move(sky);
}

void SceneSky::Reset() noexcept
{
	m_sky.reset();
}

const SceneSkyDesc* SceneSky::GetSky() const noexcept
{
	return m_sky ? &*m_sky : nullptr;
}

SceneSkyDesc* SceneSky::GetSky() noexcept
{
	return m_sky ? &*m_sky : nullptr;
}

void SceneSky::SetSky(SceneSkyDesc sky)
{
	m_sky = std::move(sky);
}

void SceneSky::RemoveSky() noexcept
{
	m_sky.reset();
}

std::optional<SceneSkyDesc> SceneSky::CaptureToDesc() const
{
	return m_sky;
}

SceneSkySnapshot SceneSky::CaptureSnapshot() const
{
	return SceneSkySnapshot{m_sky};
}
