#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCameraView.h"
#include "GameFramework/Public/Scene/Camera/SceneCameraEntry.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstddef>

class GameScene;

class SPARKLE_ENGINE_API SceneCameras final
{
  public:
	explicit SceneCameras(GameScene& scene) noexcept;
	~SceneCameras() noexcept = default;

	SceneCameras(const SceneCameras&) = delete;
	SceneCameras& operator=(const SceneCameras&) = delete;
	SceneCameras(SceneCameras&&) = delete;
	SceneCameras& operator=(SceneCameras&&) = delete;

	SceneCameraView GetActiveCamera() const noexcept;
	SceneCameraView GetCamera(std::size_t cameraIndex) const noexcept;
	SceneCameraView GetCamera(EntityId entity) const noexcept;

	std::size_t GetCameraCount() const noexcept;
	EntityId GetCameraEntity(std::size_t cameraIndex) const noexcept;
	SceneCameraEntry GetCameraEntry(std::size_t cameraIndex) const;
	SceneCameraEntry GetCameraEntryByEntity(EntityId entity) const;
	bool IsCamera(EntityId entity) const noexcept;

	void AddCamera(SceneCameraEntry&& cameraEntry);
	bool SetActiveCamera(std::size_t cameraIndex) noexcept;
	bool SetActiveCamera(EntityId entity) noexcept;
	bool SetPrimaryCameraActive() noexcept;
	void Reset(const CameraDesc& defaultCameraDesc = CameraDesc{});

  private:
	friend class GameScene;

	GameScene* m_scene = nullptr;
};
