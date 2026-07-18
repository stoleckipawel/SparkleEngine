#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraMovementSettings.h"
#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Camera/SceneCameraEntry.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "World/ECS/EntityRegistry.h"
#include "World/Resources/SceneAnimationResources.h"
#include "World/Resources/SceneDeformationStateStore.h"
#include "World/Resources/SceneMeshResources.h"
#include "World/SceneMeshInstanceData.h"

#include <optional>

class Mesh;
class SceneSkeletons;

namespace ECS
{
	class SceneWorld final
	{
	  public:
		void Clear() noexcept;
		bool IsAlive(EntityId entity) const noexcept { return m_registry.IsAlive(entity); }
		bool Destroy(EntityId entity) noexcept;

		EntityId AddCamera(SceneCameraEntry&& entry, bool active = false);
		std::size_t GetCameraCount() const noexcept;
		EntityId GetCameraEntity(std::size_t index) const noexcept;
		std::optional<SceneCameraEntry> ReadCamera(EntityId entity) const;
		bool SetActiveCamera(EntityId entity) noexcept;
		EntityId GetActiveCamera() const noexcept { return m_activeCamera; }
		bool WriteCameraDesc(EntityId entity, const CameraDesc& desc) noexcept;
		bool WriteCameraMovement(EntityId entity, const CameraMovementSettings& settings) noexcept;
		CameraMovementSettings ReadCameraMovement(EntityId entity) const noexcept;
		float ReadCameraAspectRatio(EntityId entity) const noexcept;
		bool WriteCameraAspectRatio(EntityId entity, float aspectRatio) noexcept;
		bool WriteTransform(EntityId entity, const Transform& transform) noexcept;
		Transform ReadTransform(EntityId entity) const noexcept;
		bool WriteVisibility(EntityId entity, bool visible) noexcept;
		bool ReadVisibility(EntityId entity) const noexcept;
		CameraSnapshot CaptureCamera(EntityId entity) const noexcept;

		EntityId AddMesh(SceneMeshInstanceData&& instance);
		std::size_t GetMeshCount() const noexcept;
		EntityId GetMeshEntity(std::size_t index) const noexcept;
		const Mesh* ResolveMesh(EntityId entity) const noexcept;
		bool IsSkeletalMesh(EntityId entity) const noexcept;
		MaterialHandle ReadMeshMaterial(EntityId entity) const noexcept;
		bool WriteMeshMaterial(EntityId entity, MaterialHandle material) noexcept;
		Assets::CookedAssetId ReadMeshAssetId(EntityId entity) const noexcept;
		Assets::CookedAssetId ReadSkeletonAssetId(EntityId entity) const noexcept;
		std::uint32_t ReadMeshSourceNodeIndex(EntityId entity) const noexcept;
		void AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& groups);
		std::size_t GetMeshInstanceGroupCount() const noexcept { return m_meshInstanceGroups.size(); }
		MeshSnapshot CaptureMeshes() const;
		void ApplyMorphWeights(std::span<const SceneMorphWeightSnapshot> weights);

		EntityId AddLight(SceneLightDesc&& desc);
		std::size_t GetLightCount() const noexcept;
		EntityId GetLightEntity(std::size_t index) const noexcept;
		std::optional<SceneLightDesc> ReadLight(EntityId entity) const;
		bool WriteLight(EntityId entity, SceneLightDesc&& desc);
		std::vector<SceneLightDesc> CaptureLightsToDesc() const;
		LightingSnapshot CaptureLighting() const noexcept;

		void AppendAnimationClips(std::vector<SceneAnimationClipDesc>&& clips);
		void UpdateAnimations(float deltaSeconds, const SceneSkeletons& skeletons);
		const SceneAnimationSnapshot& GetAnimationOutput() const noexcept { return m_animationResources.GetDerivedOutput(); }

	  private:
		template <typename T> std::size_t Count() const noexcept
		{
			const ComponentStorage<T>* storage = m_registry.FindStorage<T>();
			return storage == nullptr ? 0 : storage->GetEntities().size();
		}

		template <typename T> EntityId EntityAt(std::size_t index) const noexcept
		{
			const ComponentStorage<T>* storage = m_registry.FindStorage<T>();
			return storage != nullptr && index < storage->GetEntities().size() ? storage->GetEntities()[index] : EntityId::Invalid();
		}

		EntityRegistry m_registry;
		SceneMeshResources m_meshResources;
		SceneAnimationResources m_animationResources;
		SceneDeformationStateStore m_deformationStates;
		std::vector<MeshInstanceGroupSnapshot> m_meshInstanceGroups;
		EntityId m_activeCamera;
		std::uint64_t m_nextCameraIdentity = 0;
		std::uint64_t m_nextLightIdentity = 0;
	};
}
