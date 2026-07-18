#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstddef>
#include <vector>

class GameScene;
class GameSceneAssetPayloadAppender;
class Mesh;

namespace ECS
{
	struct SceneMeshInstanceData;
}

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
	void SetMaterialHandle(MaterialHandle material) noexcept;
	Assets::CookedAssetId GetMeshAssetId() const noexcept;
	Assets::CookedAssetId GetSkeletonAssetId() const noexcept;
	std::uint32_t GetSourceNodeIndex() const noexcept;

  private:
	friend class SceneMeshes;
	SceneMeshView(GameScene& scene, EntityId entity) noexcept : m_scene(&scene), m_entity(entity) {}
	GameScene* m_scene = nullptr;
	EntityId m_entity;
};

class SPARKLE_ENGINE_API SceneMeshes final
{
  public:
	explicit SceneMeshes(GameScene& scene) noexcept;
	~SceneMeshes() noexcept = default;

	SceneMeshes(const SceneMeshes&) = delete;
	SceneMeshes& operator=(const SceneMeshes&) = delete;
	SceneMeshes(SceneMeshes&&) = delete;
	SceneMeshes& operator=(SceneMeshes&&) = delete;

	std::size_t GetMeshCount() const noexcept;
	std::size_t GetMeshInstanceGroupCount() const noexcept;
	bool HasMeshes() const noexcept { return GetMeshCount() != 0; }
	EntityId GetMeshEntity(std::size_t index) const noexcept;
	SceneMeshView GetMesh(std::size_t index) const noexcept;
	SceneMeshView GetMesh(EntityId entity) const noexcept;

	bool SetMeshMaterial(SceneMeshInstanceIndex meshInstanceIndex, MaterialHandle materialHandle) noexcept;
	MeshSnapshot CaptureSnapshot() const;

  private:
	friend class GameScene;
	friend class GameSceneAssetPayloadAppender;
	bool AppendMesh(ECS::SceneMeshInstanceData&& instance);
	void AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& groups);
	GameScene* m_scene = nullptr;
};
