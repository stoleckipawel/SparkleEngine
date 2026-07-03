#include "PCH.h"
#include "Scene/Camera/SceneCameras.h"

#include <utility>

static const auto g_sceneCamerasLogger = Logging::GetOrCreateLogger("GameFramework.SceneCameras");

void SceneCameras::AppendCamera(SceneCameraEntry&& cameraEntry)
{
	m_cameraEntries.push_back(std::move(cameraEntry));
}

bool SceneCameras::ApplyCamera(std::size_t cameraIndex) noexcept
{
	if (cameraIndex >= m_cameraEntries.size())
	{
		return false;
	}

	const SceneCameraEntry& camera = m_cameraEntries[cameraIndex];
	if (!camera.IsPerspective())
	{
		return false;
	}

	m_activeCamera.ApplyFromDesc(camera.desc);
	return true;
}

bool SceneCameras::ApplyPrimaryCamera() noexcept
{
	for (std::size_t cameraIndex = 1; cameraIndex < m_cameraEntries.size(); ++cameraIndex)
	{
		if (m_cameraEntries[cameraIndex].IsPerspective())
		{
			return ApplyCamera(cameraIndex);
		}
	}

	if (!m_cameraEntries.empty())
	{
		return ApplyCamera(0);
	}
	return false;
}

void SceneCameras::Reset(const CameraDesc& defaultCameraDesc)
{
	m_activeCamera.ApplyFromDesc(defaultCameraDesc);
	m_cameraEntries.clear();

	SceneCameraEntry defaultCamera;
	defaultCamera.name = "Scene Camera";
	defaultCamera.desc = defaultCameraDesc;
	m_cameraEntries.push_back(std::move(defaultCamera));
}
