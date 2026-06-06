#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCamera.h"
#include "GameFramework/Public/Scene/Camera/SceneCameraEntry.h"

#include <cstddef>
#include <vector>

class SPARKLE_ENGINE_API SceneCameras final
{
  public:
	SceneCameras() = default;
	~SceneCameras() noexcept = default;

	SceneCameras(const SceneCameras&) = delete;
	SceneCameras& operator=(const SceneCameras&) = delete;
	SceneCameras(SceneCameras&&) = delete;
	SceneCameras& operator=(SceneCameras&&) = delete;

	SceneCamera& GetActiveCamera() noexcept { return m_activeCamera; }
	const SceneCamera& GetActiveCamera() const noexcept { return m_activeCamera; }

	std::size_t GetCameraCount() const noexcept { return m_cameraEntries.size(); }
	const std::vector<SceneCameraEntry>& GetCameraEntries() const noexcept { return m_cameraEntries; }

	void AppendCamera(SceneCameraEntry&& cameraEntry);
	bool ApplyCamera(std::size_t cameraIndex) noexcept;
	bool ApplyPrimaryCamera() noexcept;
	void Reset(const CameraDesc& defaultCameraDesc = CameraDesc{});

  private:
	SceneCamera m_activeCamera;
	std::vector<SceneCameraEntry> m_cameraEntries;
};
