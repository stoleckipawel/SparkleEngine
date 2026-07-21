#pragma once

#include "Animation/AnimationOutputStorage.h"
#include "Animation/AnimationClipResource.h"
#include "GameFramework/Public/Scene/Animations/AnimationOutput.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/Scene/Camera/CameraMovementSettings.h"
#include "GameFramework/Public/Scene/Camera/SceneCameraEntry.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/SkyEnvironment.h"
#include "GameFramework/Public/World/WorldReadView.h"
#include "World/ECS/EntityRegistry.h"
#include "World/Publication/WorldChangeJournal.h"
#include "World/Extraction/WorldExtractionStorage.h"
#include "World/Resources/MorphWeightStorage.h"
#include "World/Resources/MeshResourceStore.h"
#include "World/SceneMeshInstanceData.h"
#include "World/Systems/GameSystemGraph.h"

#include <atomic>
#include <memory>
#include <optional>

class Mesh;
class SkeletonResourceStore;
class GameWorldResourceStores;
class TaskExecutor;

namespace ECS
{
	class RenderInputExtractor;
	class SimulationSystemExecution;
	class AnimationSystemExecution;
	class TransformSystemExecution;
	class MeshExtractionSystemExecution;
	class SystemChangeCommitter;

	class GameWorldState final
	{
	  public:
		GameWorldState();
		bool IsAlive(EntityId entity) const noexcept { return m_registry.IsAlive(entity); }
		std::size_t GetEntityCount() const noexcept { return m_registry.GetLiveCount(); }
		bool Destroy(EntityId entity) noexcept;

		EntityId AddCamera(SceneCameraEntry&& entry, bool active = false);
		std::size_t GetCameraCount() const noexcept;
		EntityId GetCameraEntity(std::size_t index) const noexcept;
		bool IsCamera(EntityId entity) const noexcept;
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
		void AppendMeshInstanceGroups(std::vector<SceneMeshInstanceGroupData>&& groups);
		std::size_t GetMeshInstanceGroupCount() const noexcept { return m_meshInstanceGroups.size(); }
		std::span<const WorldExtractionStorage::MeshSlot> GetExtractedMeshes() const noexcept
		{
			return m_extraction.GetExtractedMeshes();
		}
		std::span<const SceneMeshInstanceGroupData> GetExtractedMeshGroups() const noexcept
		{
			return m_extraction.GetMeshGroups();
		}

		EntityId AddLight(SceneLightDesc&& desc);
		std::size_t GetLightCount() const noexcept;
		EntityId GetLightEntity(std::size_t index) const noexcept;
		std::optional<SceneLightDesc> ReadLight(EntityId entity) const;
		bool WriteLight(EntityId entity, SceneLightDesc&& desc);
		std::vector<SceneLightDesc> CaptureLightsToDesc() const;

		void AppendAnimationClips(
		    std::vector<AnimationClipResource>&& clips,
		    AnimationClipResourceStore& resources,
		    std::uint64_t sourceInstanceId);
		bool PrepareSystemResources(GameWorldResourceStores& resources);
		bool ExecuteSystems(
		    GameWorldResourceStores& resources,
		    TaskExecutor& executor,
		    const CameraInputIntent& cameraIntent,
		    float deltaSeconds);
		const AnimationOutput& GetAnimationOutput() const noexcept { return m_animationOutput.GetOutput(); }
		void ConfigureOscillatingMeshMotion(bool enabled);

		std::optional<SkyEnvironment> ReadSkyEnvironment() const { return m_skyEnvironment; }
		bool HasSkyEnvironment() const noexcept { return m_skyEnvironment.has_value(); }
		void WriteSkyEnvironment(SkyEnvironment environment) noexcept;
		void RemoveSkyEnvironment() noexcept;
		void NotifyResourceChanged(WorldDataKind data) noexcept;

		void CommitDerivedStateAndPublish();
		WorldReadView AcquireReadView() const noexcept;
		WorldChangeBatch ReadChanges(WorldSequence acknowledgedSequence) const;

	  private:
		friend class RenderInputExtractor;
		friend class SimulationSystemExecution;
		friend class AnimationSystemExecution;
		friend class TransformSystemExecution;
		friend class MeshExtractionSystemExecution;
		friend class SystemChangeCommitter;
		friend bool ExecuteGameWorldSystems(
		    GameWorldState&,
		    GameWorldResourceStores&,
		    TaskExecutor&,
		    const CameraInputIntent&,
		    float);

		struct SystemExecutionArena final
		{
			std::vector<EntityId> CameraChanges;
			std::vector<EntityId> MotionChanges;
			std::vector<EntityId> AnimationChanges;
			std::vector<EntityId> MorphChanges;
			std::vector<EntityId> DirtyTransforms;
			std::vector<EntityId> EvaluatedTransforms;
			std::vector<EntityId> CameraDerivedChanges;
		};

		void MarkTransformDirty(EntityId entity) noexcept;
		void RecordChange(EntityId entity, WorldChangeKind kind, WorldDataKind data) noexcept;
		void PublishPendingChanges();
		void PublishReadView(std::span<const WorldChange> changes, WorldSequence sequence, bool fullBaseline);
		WorldCameraReadData BuildCameraReadData(EntityId entity) const;
		WorldLightReadData BuildLightReadData(EntityId entity) const;
		WorldMeshReadData BuildMeshReadData(EntityId entity) const;
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
		MeshResourceStore m_meshResources;
		AnimationOutputStorage m_animationOutput;
		MorphWeightStorage m_morphWeights;
		WorldExtractionStorage m_extraction;
		SystemExecutionArena m_systemArena;
		CompiledGameSystemGraph m_systemGraph;
		std::vector<SceneMeshInstanceGroupData> m_meshInstanceGroups;
		std::optional<SkyEnvironment> m_skyEnvironment;
		std::vector<EntityId> m_dirtyTransforms;
		std::vector<WorldChange> m_pendingChanges;
		WorldChangeJournal m_changeJournal;
		std::atomic<std::shared_ptr<const WorldReadView::Storage>> m_publishedReadView;
		std::uint64_t m_readGeneration = 0;
		bool m_forceFullPublication = true;
		bool m_evaluateAllTransforms = false;
		bool m_changesOverflowed = false;
		EntityId m_activeCamera;
		std::uint64_t m_nextCameraIdentity = 0;
		std::uint64_t m_nextLightIdentity = 0;
		float m_motionTimeSeconds = 0.0f;
		bool m_oscillatingMeshMotionEnabled = false;
	};
}
