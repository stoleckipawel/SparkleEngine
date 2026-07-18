#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraMovementSettings.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/EntityId.h"

class GameScene;

// A non-owning, entity-bound view of camera instance data. Camera navigation and
// input behavior belong to a controller; collection and active-camera policy
// belong to SceneCameras.
class SPARKLE_ENGINE_API SceneCameraView final
{
  public:
	SceneCameraView() noexcept = default;

	bool IsValid() const noexcept;
	EntityId GetEntity() const noexcept { return m_entity; }
	CameraDesc GetDesc() const noexcept;
	void SetDesc(const CameraDesc& desc) noexcept;
	CameraMovementSettings GetMovementSettings() const noexcept;
	void SetMovementSettings(const CameraMovementSettings& settings) noexcept;
	Transform GetTransform() const noexcept;
	void SetTransform(const Transform& transform) noexcept;
	bool IsVisible() const noexcept;
	void SetVisible(bool visible) noexcept;
	float GetAspectRatio() const noexcept;
	void SetAspectRatio(float aspectRatio) noexcept;

  private:
	friend class SceneCameras;
	SceneCameraView(GameScene& scene, EntityId entity) noexcept : m_scene(&scene), m_entity(entity) {}

	GameScene* m_scene = nullptr;
	EntityId m_entity;
};
