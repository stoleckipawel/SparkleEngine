#include "PCH.h"
#include "Scene/Camera/SceneCameras.h"

#include "Scene/GameScene.h"
#include "World/SceneWorld.h"

SceneCameras::SceneCameras(GameScene& scene) noexcept : m_scene(&scene) {}

SceneCameraView SceneCameras::GetActiveCamera() const noexcept
{
	return GetCamera(m_scene->m_world->GetActiveCamera());
}

SceneCameraView SceneCameras::GetCamera(std::size_t cameraIndex) const noexcept
{
	return GetCamera(GetCameraEntity(cameraIndex));
}

SceneCameraView SceneCameras::GetCamera(EntityId entity) const noexcept
{
	return IsCamera(entity) ? SceneCameraView(*m_scene, entity) : SceneCameraView{};
}

std::size_t SceneCameras::GetCameraCount() const noexcept { return m_scene->m_world->GetCameraCount(); }

EntityId SceneCameras::GetCameraEntity(std::size_t cameraIndex) const noexcept
{
	return m_scene->m_world->GetCameraEntity(cameraIndex);
}

SceneCameraEntry SceneCameras::GetCameraEntry(std::size_t cameraIndex) const
{
	const std::optional<SceneCameraEntry> entry = m_scene->m_world->ReadCamera(GetCameraEntity(cameraIndex));
	return entry.value_or(SceneCameraEntry{});
}

SceneCameraEntry SceneCameras::GetCameraEntryByEntity(EntityId entity) const
{
	return m_scene->m_world->ReadCamera(entity).value_or(SceneCameraEntry{});
}

bool SceneCameras::IsCamera(EntityId entity) const noexcept { return m_scene->m_world->ReadCamera(entity).has_value(); }

void SceneCameras::AddCamera(SceneCameraEntry&& cameraEntry)
{
	m_scene->m_world->AddCamera(std::move(cameraEntry));
}

bool SceneCameras::SetActiveCamera(std::size_t cameraIndex) noexcept { return SetActiveCamera(GetCameraEntity(cameraIndex)); }

bool SceneCameras::SetActiveCamera(EntityId entity) noexcept { return m_scene->m_world->SetActiveCamera(entity); }

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
		m_scene->m_world->AddCamera(std::move(entry), true);
		return;
	}
	m_scene->m_world->WriteCameraDesc(m_scene->m_world->GetActiveCamera(), defaultCameraDesc);
}
