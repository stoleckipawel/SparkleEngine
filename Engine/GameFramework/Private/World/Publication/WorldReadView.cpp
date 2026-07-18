#include "PCH.h"
#include "GameFramework/Public/World/WorldReadView.h"
#include "World/Publication/WorldReadViewInternal.h"

std::uint64_t WorldReadView::GetGeneration() const noexcept { return m_storage != nullptr ? m_storage->Generation : 0; }
WorldSequence WorldReadView::GetSequence() const noexcept { return m_storage != nullptr ? m_storage->Sequence : 0; }

std::span<const WorldCameraReadData> WorldReadView::GetCameras() const noexcept
{
	return m_storage != nullptr ? std::span<const WorldCameraReadData>(m_storage->Cameras) : std::span<const WorldCameraReadData>{};
}

std::span<const WorldLightReadData> WorldReadView::GetLights() const noexcept
{
	return m_storage != nullptr ? std::span<const WorldLightReadData>(m_storage->Lights) : std::span<const WorldLightReadData>{};
}

std::span<const WorldMeshReadData> WorldReadView::GetMeshes() const noexcept
{
	return m_storage != nullptr ? std::span<const WorldMeshReadData>(m_storage->Meshes) : std::span<const WorldMeshReadData>{};
}

const std::optional<SkyEnvironment>& WorldReadView::GetSkyEnvironment() const noexcept
{
	static const std::optional<SkyEnvironment> Empty;
	return m_storage != nullptr ? m_storage->Sky : Empty;
}
