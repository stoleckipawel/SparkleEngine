#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Component.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <DirectXMath.h>
#include <memory>

class Mesh;

class SPARKLE_ENGINE_API MeshComponent : public Component
{
  public:
	explicit MeshComponent(std::unique_ptr<Mesh>&& mesh) noexcept;
	MeshComponent(
	    std::unique_ptr<Mesh>&& mesh,
	    SceneMeshKind meshKind,
	    const Transform& transform,
	    MaterialHandle materialHandle = MaterialHandle::Invalid(),
	    Assets::CookedAssetId meshAssetId = Assets::InvalidCookedAssetId,
	    SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex,
	    SceneMeshInstanceGroupIndex meshInstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex,
	    Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId,
	    std::uint32_t sourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex) noexcept;
	~MeshComponent() override;

	MeshComponent(const MeshComponent&) = delete;
	MeshComponent& operator=(const MeshComponent&) = delete;
	MeshComponent(MeshComponent&&) noexcept = default;
	MeshComponent& operator=(MeshComponent&&) noexcept = default;

	Mesh* GetMesh() noexcept { return m_mesh.get(); }
	const Mesh* GetMesh() const noexcept { return m_mesh.get(); }
	bool HasMesh() const noexcept { return m_mesh != nullptr; }

	void SetTransform(const Transform& transform) noexcept { m_transform = transform; }
	Transform& GetTransform() noexcept { return m_transform; }
	const Transform& GetTransform() const noexcept { return m_transform; }

	void SetMaterialHandle(MaterialHandle materialHandle) noexcept { m_materialHandle = materialHandle; }
	MaterialHandle GetMaterialHandle() const noexcept { return m_materialHandle; }
	void SetMeshAssetId(Assets::CookedAssetId meshAssetId) noexcept { m_meshAssetId = meshAssetId; }
	Assets::CookedAssetId GetMeshAssetId() const noexcept { return m_meshAssetId; }
	void SetMeshAssetIndex(SceneMeshAssetIndex meshAssetIndex) noexcept { m_meshAssetIndex = meshAssetIndex; }
	SceneMeshAssetIndex GetMeshAssetIndex() const noexcept { return m_meshAssetIndex; }
	void SetMeshInstanceGroupIndex(SceneMeshInstanceGroupIndex meshInstanceGroupIndex) noexcept
	{
		m_meshInstanceGroupIndex = meshInstanceGroupIndex;
	}
	SceneMeshInstanceGroupIndex GetMeshInstanceGroupIndex() const noexcept { return m_meshInstanceGroupIndex; }
	void SetSkeletonAssetId(Assets::CookedAssetId skeletonAssetId) noexcept { m_skeletonAssetId = skeletonAssetId; }
	Assets::CookedAssetId GetSkeletonAssetId() const noexcept { return m_skeletonAssetId; }
	std::uint32_t GetSourceNodeIndex() const noexcept { return m_sourceNodeIndex; }
	SceneMeshKind GetMeshKind() const noexcept { return m_kind; }
	bool IsSkeletalMeshComponent() const noexcept { return m_kind == SceneMeshKind::Skeletal; }

	DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;

  private:
	std::unique_ptr<Mesh> m_mesh;
	Transform m_transform;
	MaterialHandle m_materialHandle = MaterialHandle::Invalid();
	Assets::CookedAssetId m_meshAssetId = Assets::InvalidCookedAssetId;
	SceneMeshAssetIndex m_meshAssetIndex = kInvalidSceneMeshAssetIndex;
	SceneMeshInstanceGroupIndex m_meshInstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
	Assets::CookedAssetId m_skeletonAssetId = Assets::InvalidCookedAssetId;
	std::uint32_t m_sourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex;
	SceneMeshKind m_kind = SceneMeshKind::Static;
};
