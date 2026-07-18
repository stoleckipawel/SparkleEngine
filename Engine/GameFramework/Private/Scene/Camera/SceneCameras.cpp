#include "PCH.h"
#include "Scene/Camera/SceneCameras.h"

#include "World/GameWorld.h"
#include "World/GameWorldState.h"

SceneCameras::SceneCameras(GameWorld& world) noexcept : m_world(&world) {}

SceneCameraView SceneCameras::GetActiveCamera() const noexcept
{
	return GetCamera(m_world->m_state->GetActiveCamera());
}

SceneCameraView SceneCameras::GetCamera(std::size_t cameraIndex) const noexcept
{
	return GetCamera(GetCameraEntity(cameraIndex));
}

SceneCameraView SceneCameras::GetCamera(EntityId entity) const noexcept
{
	return IsCamera(entity) ? SceneCameraView(*m_world, entity) : SceneCameraView{};
}

std::size_t SceneCameras::GetCameraCount() const noexcept { return m_world->m_state->GetCameraCount(); }

EntityId SceneCameras::GetCameraEntity(std::size_t cameraIndex) const noexcept
{
	return m_world->m_state->GetCameraEntity(cameraIndex);
}

SceneCameraEntry SceneCameras::GetCameraEntry(std::size_t cameraIndex) const
{
	const std::optional<SceneCameraEntry> entry = m_world->m_state->ReadCamera(GetCameraEntity(cameraIndex));
	return entry.value_or(SceneCameraEntry{});
}

SceneCameraEntry SceneCameras::GetCameraEntryByEntity(EntityId entity) const
{
	return m_world->m_state->ReadCamera(entity).value_or(SceneCameraEntry{});
}

bool SceneCameras::IsCamera(EntityId entity) const noexcept { return m_world->m_state->IsCamera(entity); }

void SceneCameras::AddCamera(SceneCameraEntry&& cameraEntry)
{
	m_world->m_state->AddCamera(std::move(cameraEntry));
}

bool SceneCameras::SetActiveCamera(std::size_t cameraIndex) noexcept { return SetActiveCamera(GetCameraEntity(cameraIndex)); }

bool SceneCameras::SetActiveCamera(EntityId entity) noexcept { return m_world->m_state->SetActiveCamera(entity); }

bool SceneCameras::SetPrimaryCameraActive() noexcept
{
	for (std::size_t cameraIndex = 1; cameraIndex < GetCameraCount(); ++cameraIndex)
	{
		if (GetCameraEntry(cameraIndex).IsPerspective())
		{
			return SetActiveCamera(cameraIndex);
		}
	}
	return GetCameraCount() != 0 && SetActiveCamera(0);
}

void SceneCameras::Reset(const CameraDesc& defaultCameraDesc)
{
	if (GetCameraCount() == 0)
	{
		SceneCameraEntry entry;
		entry.name = "Scene Camera";
		entry.desc = defaultCameraDesc;
		m_world->m_state->AddCamera(std::move(entry), true);
		return;
	}
	m_world->m_state->WriteCameraDesc(m_world->m_state->GetActiveCamera(), defaultCameraDesc);
}
