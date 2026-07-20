#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstddef>

class GameWorld;
class Mesh;

class SPARKLE_ENGINE_API SceneMeshView final
{
  public:
	SceneMeshView() noexcept = default;
	bool IsValid() const noexcept;
	EntityId GetEntity() const noexcept { return m_entity; }
	const Mesh* GetMesh() const noexcept;
	bool IsVisible() const noexcept;
	void SetVisible(bool visible) noexcept;
	bool IsSkeletal() const noexcept;
	Transform GetTransform() const noexcept;
	void SetTransform(const Transform& transform) noexcept;
	MaterialHandle GetMaterialHandle() const noexcept;
	Assets::CookedAssetId GetMeshAssetId() const noexcept;
	Assets::CookedAssetId GetSkeletonAssetId() const noexcept;
	std::uint32_t GetSourceNodeIndex() const noexcept;

  private:
	friend class SceneMeshes;
	SceneMeshView(GameWorld& world, EntityId entity) noexcept : m_world(&world), m_entity(entity) {}
	GameWorld* m_world = nullptr;
	EntityId m_entity;
};

class SPARKLE_ENGINE_API SceneMeshes final
{
  public:
	explicit SceneMeshes(GameWorld& world) noexcept;
	~SceneMeshes() noexcept = default;

	SceneMeshes(const SceneMeshes&) = delete;
	SceneMeshes& operator=(const SceneMeshes&) = delete;
	SceneMeshes(SceneMeshes&&) = delete;
	SceneMeshes& operator=(SceneMeshes&&) = delete;

	std::size_t GetMeshCount() const noexcept;
	bool HasMeshes() const noexcept { return GetMeshCount() != 0; }
	EntityId GetMeshEntity(std::size_t index) const noexcept;
	SceneMeshView GetMesh(std::size_t index) const noexcept;
	SceneMeshView GetMesh(EntityId entity) const noexcept;

	MeshSnapshot CaptureSnapshot() const;

  private:
	friend class GameWorld;
	GameWorld* m_world = nullptr;
};
