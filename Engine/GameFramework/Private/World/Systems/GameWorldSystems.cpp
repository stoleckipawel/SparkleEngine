#include "PCH.h"

#include "World/Systems/GameWorldSystems.h"

#include "Animation/AnimationOutputStorage.h"
#include "Animation/AnimationPoseEvaluator.h"
#include "Animation/MorphWeightEvaluator.h"
#include "Animation/SkinningMatrixEvaluator.h"
#include "Tasks/Public/TaskExecutor.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/MotionComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/Query.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/Systems/CameraDerivedStateEvaluationSystem.h"
#include "World/Systems/CameraMovementSystem.h"
#include "World/Systems/GameSystemGraph.h"
#include "World/Systems/OscillatingMeshMotionSystem.h"
#include "World/Systems/TransformEvaluationSystem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace ECS
{
	namespace GameWorldSystemIds
	{
		constexpr GameSystemId CameraMovement = MakeGameSystemId("Sparkle.Game.CameraMovement");
		constexpr GameSystemId OscillatingMeshMotion = MakeGameSystemId("Sparkle.Game.OscillatingMeshMotion");
		constexpr GameSystemId PlaybackAdvance = MakeGameSystemId("Sparkle.Game.AnimationPlaybackAdvance");
		constexpr GameSystemId PoseEvaluation = MakeGameSystemId("Sparkle.Game.AnimationPoseEvaluation");
		constexpr GameSystemId MorphWeightEvaluation = MakeGameSystemId("Sparkle.Game.MorphWeightEvaluation");
		constexpr GameSystemId SkinningMatrixEvaluation = MakeGameSystemId("Sparkle.Game.SkinningMatrixEvaluation");
		constexpr GameSystemId MorphOutputCommit = MakeGameSystemId("Sparkle.Game.MorphOutputCommit");
		constexpr GameSystemId SystemOutputCommit = MakeGameSystemId("Sparkle.Game.SystemOutputCommit");
		constexpr GameSystemId TransformEvaluation = MakeGameSystemId("Sparkle.Game.TransformEvaluation");
		constexpr GameSystemId CameraDerivedState = MakeGameSystemId("Sparkle.Game.CameraDerivedState");
		constexpr GameSystemId MeshExtraction = MakeGameSystemId("Sparkle.Game.MeshExtraction");
		constexpr GameSystemId ExtractionCommit = MakeGameSystemId("Sparkle.Game.ExtractionCommit");
	}

	using CameraMovementQuery = Query<Write<Camera>, Write<CameraMovement>, Write<LocalTransform>>;
	using OscillatingMotionQuery = Query<Read<OscillatingMotion>, Write<LocalTransform>>;
	using PlaybackAdvanceQuery = Query<Write<AnimationState>>;
	using PoseEvaluationQuery = Query<Read<AnimationState>>;
	using MorphEvaluationQuery = Query<Read<AnimationState>>;
	using TransformEvaluationQuery = Query<Read<LocalTransform>, Write<WorldTransform>>;
	using CameraDerivedStateQuery = Query<Read<LocalTransform>, Read<Camera>, Write<CameraDerivedState>>;
	using MeshExtractionQuery = Query<Read<MeshInstance>, Read<Visibility>, Read<WorldTransform>>;

	namespace
	{
		constexpr ParallelForPolicy CameraGrain{.GrainSize = 32, .SerialThreshold = 64, .MaximumPartitions = 4};
		constexpr ParallelForPolicy MotionGrain{.GrainSize = 32, .SerialThreshold = 64, .MaximumPartitions = 8};
		constexpr ParallelForPolicy AnimationGrain{.GrainSize = 8, .SerialThreshold = 16, .MaximumPartitions = 16};
		constexpr ParallelForPolicy PoseGrain{.GrainSize = 2, .SerialThreshold = 4, .MaximumPartitions = 16};
		constexpr ParallelForPolicy TransformGrain{.GrainSize = 64, .SerialThreshold = 128, .MaximumPartitions = 16};
		constexpr ParallelForPolicy ExtractionGrain{.GrainSize = 64, .SerialThreshold = 128, .MaximumPartitions = 16};
		constexpr ParallelForPolicy SingleItemGrain{.GrainSize = 1, .SerialThreshold = 1, .MaximumPartitions = 1};

		GameSystemExecutionPolicy Ranges(ParallelForPolicy policy) noexcept
		{
			return GameSystemExecutionPolicy{GameSystemExecutionMode::ParallelRanges, policy};
		}

		void SortUnique(std::vector<EntityId>& entities)
		{
			std::sort(entities.begin(), entities.end());
			entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
		}

		template <typename T> std::uint32_t StorageCount(const EntityRegistry& registry) noexcept
		{
			const ComponentStorage<T>* storage = registry.FindStorage<T>();
			return storage == nullptr ? 0u : static_cast<std::uint32_t>(storage->GetEntities().size());
		}
	}

	class GameWorldSystemRun final
	{
	  public:
		GameWorldSystemRun(
		    GameWorldState& state,
		    GameWorldResourceStores& resources,
		    const CameraInputIntent& cameraIntent,
		    float deltaSeconds,
		    const StructureFrozenEpoch& epoch) :
		    m_state(state),
		    m_resources(resources),
		    m_cameraIntent(cameraIntent),
		    m_deltaSeconds((std::max)(0.0f, deltaSeconds)),
		    m_nextMotionTime(state.m_motionTimeSeconds + m_deltaSeconds),
		    m_cameraQuery(state.m_registry, epoch),
		    m_motionQuery(state.m_registry, epoch),
		    m_playbackQuery(state.m_registry, epoch),
		    m_poseQuery(state.m_registry, epoch),
		    m_transformQuery(state.m_registry, epoch),
		    m_cameraDerivedQuery(state.m_registry, epoch),
		    m_extractionQuery(state.m_registry, epoch)
		{
			m_cameraQuery.PrepareWriteTraversal();
			m_motionQuery.PrepareWriteTraversal();
			m_playbackQuery.PrepareWriteTraversal();
			m_transformQuery.PrepareWriteTraversal();
			m_cameraDerivedQuery.PrepareWriteTraversal();
			BuildBindings();
		}

		std::span<const GameSystemExecutionBinding> GetBindings() const noexcept { return m_bindings; }

	  private:
		bool RunCamera(std::uint32_t begin, std::uint32_t end)
		{
			return m_cameraQuery.ForEachRange(
			                           begin,
			                           end,
			                           [this](std::size_t index, EntityId entity, Camera& camera, CameraMovement& movement, LocalTransform& transform)
			                           {
				                           if (CameraMovementSystem::Apply(
				                                   entity,
				                                   m_state.m_activeCamera,
				                                   m_cameraIntent,
				                                   m_deltaSeconds,
				                                   camera,
				                                   movement,
				                                   transform))
				                           {
					                           m_state.m_systemArena.CameraChanges[index] = entity;
				                           }
			                           })
			    .Succeeded();
		}

		bool RunMotion(std::uint32_t begin, std::uint32_t end)
		{
			const bool useLanes = m_motionQuery.GetEstimatedEntityCount() > 1;
			return m_motionQuery.ForEachRange(
			                           begin,
			                           end,
			                           [this, useLanes](std::size_t index, EntityId entity, const OscillatingMotion& motion, LocalTransform& transform)
			                           {
				                           OscillatingMeshMotionSystem::Apply(motion, m_nextMotionTime, useLanes, transform);
				                           m_state.m_systemArena.MotionChanges[index] = entity;
			                           })
			    .Succeeded();
		}

		bool RunPlayback(std::uint32_t begin, std::uint32_t end)
		{
			return m_playbackQuery.ForEachRange(
			                             begin,
			                             end,
			                             [this](std::size_t index, EntityId entity, AnimationState& state)
			                             {
				                             const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(state.Resource);
				                             if (!clip.IsValid() || !state.Playing || clip.Resource->durationSeconds <= 0.0f)
					                             return;
				                             state.TimeSeconds += m_deltaSeconds * state.PlaybackRate;
				                             state.TimeSeconds = state.Looping
				                                                     ? std::fmod(state.TimeSeconds, clip.Resource->durationSeconds)
				                                                     : (std::min)(state.TimeSeconds, clip.Resource->durationSeconds);
				                             m_state.m_systemArena.AnimationChanges[index] = entity;
			                             })
			    .Succeeded();
		}

		bool RunPose(std::uint32_t begin, std::uint32_t end)
		{
			return m_poseQuery.ForEachRange(
			                         begin,
			                         end,
			                         [this](std::size_t, EntityId entity, const AnimationState& state)
			                         {
				                         AnimationOutputStorage::PoseWorkSlot* work = m_state.m_animationOutput.FindPoseWork(entity);
				                         if (work == nullptr || work->PoseOutputIndex >= m_state.m_animationOutput.GetMutableOutput().poses.size())
					                         return;
				                         const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(state.Resource);
				                         const SkeletonEvaluationData skeleton = m_resources.Skeletons.Resolve(work->Skeleton);
				                         if (!clip.IsValid() || !AnimationPoseEvaluator::Evaluate(
				                                                    *clip.Resource,
				                                                    skeleton,
				                                                    state.TimeSeconds,
				                                                    work->LocalTransforms,
				                                                    work->ModelSpaceTransforms))
					                         return;
				                         m_state.m_animationOutput.GetMutableOutput().poses[work->PoseOutputIndex].playbackTimeSeconds = state.TimeSeconds;
			                         })
			    .Succeeded();
		}

		bool RunMorphSamples(std::uint32_t begin, std::uint32_t end)
		{
			std::span<AnimationOutputStorage::MorphSampleSlot> samples = m_state.m_animationOutput.GetMorphSamples();
			AnimationOutput& output = m_state.m_animationOutput.GetMutableOutput();
			for (std::uint32_t index = begin; index < end; ++index)
			{
				const AnimationOutputStorage::MorphSampleSlot& sample = samples[index];
				const AnimationState* state = m_state.m_registry.Get<AnimationState>(sample.AnimationEntity);
				const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(sample.Clip);
				if (state == nullptr || !clip.IsValid() || sample.OutputIndex >= output.morphWeights.size() ||
				    !MorphWeightEvaluator::Evaluate(
				        *clip.Resource,
				        sample.ChannelIndex,
				        state->TimeSeconds,
				        output.morphWeights[sample.OutputIndex].weights))
					return false;
			}
			return true;
		}

		bool RunSkinning(std::uint32_t begin, std::uint32_t end)
		{
			std::span<AnimationOutputStorage::PoseWorkSlot> work = m_state.m_animationOutput.GetPoseWork();
			AnimationOutput& output = m_state.m_animationOutput.GetMutableOutput();
			for (std::uint32_t index = begin; index < end; ++index)
			{
				AnimationOutputStorage::PoseWorkSlot& slot = work[index];
				if (slot.PoseOutputIndex >= output.poses.size())
					continue;
				if (!SkinningMatrixEvaluator::Evaluate(
				        m_resources.Skeletons.Resolve(slot.Skeleton),
				        slot.ModelSpaceTransforms,
				        output.poses[slot.PoseOutputIndex].skinningMatrices))
					return false;
			}
			return true;
		}

		bool CommitMorphOutputs(std::uint32_t, std::uint32_t)
		{
			const auto bindings = m_state.m_animationOutput.GetMorphBindings();
			const auto samples = m_state.m_animationOutput.GetMorphSamples();
			const AnimationOutput& output = m_state.m_animationOutput.GetOutput();
			for (std::size_t index = 0; index < bindings.size(); ++index)
			{
				const AnimationOutputStorage::MorphTargetBinding& binding = bindings[index];
				if (binding.SampleIndex >= samples.size())
					return false;
				const std::uint32_t outputIndex = samples[binding.SampleIndex].OutputIndex;
				if (outputIndex >= output.morphWeights.size() ||
				    !m_state.m_morphWeights.Write(binding.TargetWeights, output.morphWeights[outputIndex].weights))
					return false;
				m_state.m_systemArena.MorphChanges[index] = binding.TargetEntity;
			}
			return true;
		}

		bool CommitSystemOutputs(std::uint32_t, std::uint32_t)
		{
			auto recordTransforms = [this](std::vector<EntityId>& changes)
			{
				std::erase(changes, EntityId::Invalid());
				SortUnique(changes);
				for (EntityId entity : changes)
				{
					m_state.m_systemArena.DirtyTransforms.push_back(entity);
					m_state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::LocalTransform);
				}
			};
			recordTransforms(m_state.m_systemArena.CameraChanges);
			recordTransforms(m_state.m_systemArena.MotionChanges);
			std::erase(m_state.m_systemArena.AnimationChanges, EntityId::Invalid());
			SortUnique(m_state.m_systemArena.AnimationChanges);
			for (EntityId entity : m_state.m_systemArena.AnimationChanges)
				m_state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::AnimationState);
			std::erase(m_state.m_systemArena.MorphChanges, EntityId::Invalid());
			SortUnique(m_state.m_systemArena.MorphChanges);
			for (EntityId entity : m_state.m_systemArena.MorphChanges)
				m_state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::MorphState);
			SortUnique(m_state.m_systemArena.DirtyTransforms);
			m_state.m_motionTimeSeconds = m_nextMotionTime;
			return true;
		}

		bool RunTransforms(std::uint32_t begin, std::uint32_t end)
		{
			return m_transformQuery.ForEachEntityRange(
			                              m_state.m_systemArena.DirtyTransforms,
			                              begin,
			                              end,
			                              [](std::size_t, EntityId, const LocalTransform& local, WorldTransform& world)
			                              {
				                              TransformEvaluationSystem::Evaluate(local, world);
			                              })
			    .Succeeded();
		}

		bool RunCameraDerivedState(std::uint32_t begin, std::uint32_t end)
		{
			return m_cameraDerivedQuery.ForEachEntityRange(
			                                    m_state.m_systemArena.DirtyTransforms,
			                                    begin,
			                                    end,
			                                    [](std::size_t, EntityId, const LocalTransform& local, const Camera&, CameraDerivedState& derived)
			                                    {
				                                    CameraDerivedStateEvaluationSystem::Evaluate(local, derived);
			                                    })
			    .Succeeded();
		}

		bool RunMeshExtraction(std::uint32_t begin, std::uint32_t end)
		{
			std::span<WorldExtractionStorage::MeshSlot> slots = m_state.m_extraction.GetMeshSlots();
			return m_extractionQuery.ForEachRange(
			                               begin,
			                               end,
			                               [this, slots](std::size_t index, EntityId entity, const MeshInstance& mesh, const Visibility& visibility, const WorldTransform& world)
			                               {
				                               WorldExtractionStorage::MeshSlot& slot = slots[index];
				                               slot.Entity = entity;
				                               const Mesh* resource = m_state.m_meshResources.Resolve(mesh.Resource);
				                               slot.Included = visibility.Visible && resource != nullptr;
				                               if (!slot.Included)
					                               return;
				                               slot.Snapshot.mesh = resource;
				                               slot.Snapshot.worldMatrix = world.Matrix;
				                               DirectX::XMStoreFloat3x4(
				                                   &slot.Snapshot.worldInvTranspose,
				                                   DirectX::XMLoadFloat4x4(&world.InverseTranspose));
				                               slot.Snapshot.materialHandle = mesh.Material;
				                               slot.Snapshot.meshAssetId = mesh.MeshAssetId;
				                               slot.Snapshot.skeletonAssetId = mesh.SkeletonAssetId;
				                               slot.Snapshot.meshKind = mesh.Kind;
				                               slot.Snapshot.meshAssetIndex = mesh.MeshAssetIndex;
				                               slot.Snapshot.instanceGroupIndex = mesh.InstanceGroupIndex;
			                               })
			    .Succeeded();
		}

		bool CommitExtraction(std::uint32_t, std::uint32_t)
		{
			m_state.m_extraction.CommitMeshes(m_state.m_meshInstanceGroups);
			for (EntityId entity : m_state.m_systemArena.DirtyTransforms)
			{
				if (!m_state.m_registry.IsAlive(entity))
					continue;
				m_state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::WorldTransform);
				if (m_state.m_registry.Get<Camera>(entity) != nullptr)
					m_state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::CameraDerivedState);
			}
			m_state.PublishPendingChanges();
			return true;
		}

		void BuildBindings()
		{
			m_bindings = {{
			    {GameWorldSystemIds::CameraMovement, [this] { return static_cast<std::uint32_t>(m_cameraQuery.GetEstimatedEntityCount()); }, [this](auto b, auto e) { return RunCamera(b, e); }},
			    {GameWorldSystemIds::OscillatingMeshMotion, [this] { return static_cast<std::uint32_t>(m_motionQuery.GetEstimatedEntityCount()); }, [this](auto b, auto e) { return RunMotion(b, e); }},
			    {GameWorldSystemIds::PlaybackAdvance, [this] { return static_cast<std::uint32_t>(m_playbackQuery.GetEstimatedEntityCount()); }, [this](auto b, auto e) { return RunPlayback(b, e); }},
			    {GameWorldSystemIds::PoseEvaluation, [this] { return static_cast<std::uint32_t>(m_poseQuery.GetEstimatedEntityCount()); }, [this](auto b, auto e) { return RunPose(b, e); }},
			    {GameWorldSystemIds::MorphWeightEvaluation, [this] { return static_cast<std::uint32_t>(m_state.m_animationOutput.GetMorphSamples().size()); }, [this](auto b, auto e) { return RunMorphSamples(b, e); }},
			    {GameWorldSystemIds::SkinningMatrixEvaluation, [this] { return static_cast<std::uint32_t>(m_state.m_animationOutput.GetPoseWork().size()); }, [this](auto b, auto e) { return RunSkinning(b, e); }},
			    {GameWorldSystemIds::MorphOutputCommit, [] { return 1u; }, [this](auto b, auto e) { return CommitMorphOutputs(b, e); }},
			    {GameWorldSystemIds::SystemOutputCommit, [] { return 1u; }, [this](auto b, auto e) { return CommitSystemOutputs(b, e); }},
			    {GameWorldSystemIds::TransformEvaluation, [this] { return static_cast<std::uint32_t>(m_state.m_systemArena.DirtyTransforms.size()); }, [this](auto b, auto e) { return RunTransforms(b, e); }},
			    {GameWorldSystemIds::CameraDerivedState, [this] { return static_cast<std::uint32_t>(m_state.m_systemArena.DirtyTransforms.size()); }, [this](auto b, auto e) { return RunCameraDerivedState(b, e); }},
			    {GameWorldSystemIds::MeshExtraction, [this] { return static_cast<std::uint32_t>(m_extractionQuery.GetEstimatedEntityCount()); }, [this](auto b, auto e) { return RunMeshExtraction(b, e); }},
			    {GameWorldSystemIds::ExtractionCommit, [] { return 1u; }, [this](auto b, auto e) { return CommitExtraction(b, e); }},
			}};
		}

		GameWorldState& m_state;
		GameWorldResourceStores& m_resources;
		const CameraInputIntent& m_cameraIntent;
		float m_deltaSeconds = 0.0f;
		float m_nextMotionTime = 0.0f;
		CameraMovementQuery m_cameraQuery;
		OscillatingMotionQuery m_motionQuery;
		PlaybackAdvanceQuery m_playbackQuery;
		PoseEvaluationQuery m_poseQuery;
		TransformEvaluationQuery m_transformQuery;
		CameraDerivedStateQuery m_cameraDerivedQuery;
		MeshExtractionQuery m_extractionQuery;
		std::array<GameSystemExecutionBinding, 12> m_bindings;
	};

	CompiledGameSystemGraph BuildGameWorldSystemGraph()
	{
		GameSystemGraph graph;
		GameSystemDesc camera{GameWorldSystemIds::CameraMovement, "Game.CameraMovement", GameSystemPhase::Simulation};
		camera.DeclareQuery<CameraMovementQuery>();
		camera.Resources = {
		    {GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::CameraInputIntent, GameSystemAccessMode::Read}};
		camera.Execution = Ranges(CameraGrain);
		graph.Add(std::move(camera));

		GameSystemDesc motion{GameWorldSystemIds::OscillatingMeshMotion, "Game.OscillatingMeshMotion", GameSystemPhase::Simulation};
		motion.DeclareQuery<OscillatingMotionQuery>();
		motion.Resources = {
		    {GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Write}};
		motion.Prerequisites = {GameWorldSystemIds::CameraMovement};
		motion.Execution = Ranges(MotionGrain);
		graph.Add(std::move(motion));

		GameSystemDesc playback{GameWorldSystemIds::PlaybackAdvance, "Game.AnimationPlaybackAdvance", GameSystemPhase::Animation};
		playback.DeclareQuery<PlaybackAdvanceQuery>();
		playback.Resources = {
		    {GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read}};
		playback.Execution = Ranges(AnimationGrain);
		graph.Add(std::move(playback));

		GameSystemDesc pose{GameWorldSystemIds::PoseEvaluation, "Game.AnimationPoseEvaluation", GameSystemPhase::Animation};
		pose.DeclareQuery<PoseEvaluationQuery>();
		pose.Resources = {
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Write}};
		pose.Prerequisites = {GameWorldSystemIds::PlaybackAdvance};
		pose.Execution = Ranges(PoseGrain);
		graph.Add(std::move(pose));

		GameSystemDesc morph{GameWorldSystemIds::MorphWeightEvaluation, "Game.MorphWeightEvaluation", GameSystemPhase::Animation};
		morph.DeclareQuery<MorphEvaluationQuery>();
		morph.Resources = {
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Write}};
		morph.Prerequisites = {GameWorldSystemIds::PlaybackAdvance};
		morph.Execution = Ranges(AnimationGrain);
		graph.Add(std::move(morph));

		GameSystemDesc skinning{GameWorldSystemIds::SkinningMatrixEvaluation, "Game.SkinningMatrixEvaluation", GameSystemPhase::Deformation};
		skinning.Resources = {
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Write}};
		skinning.Prerequisites = {GameWorldSystemIds::PoseEvaluation};
		skinning.Execution = Ranges(PoseGrain);
		graph.Add(std::move(skinning));

		GameSystemDesc morphCommit{GameWorldSystemIds::MorphOutputCommit, "Game.MorphOutputCommit", GameSystemPhase::Deformation};
		morphCommit.Resources = {
		    {GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Write}};
		morphCommit.Prerequisites = {GameWorldSystemIds::MorphWeightEvaluation};
		morphCommit.Execution = Ranges(SingleItemGrain);
		graph.Add(std::move(morphCommit));

		GameSystemDesc outputCommit{GameWorldSystemIds::SystemOutputCommit, "Game.SystemOutputCommit", GameSystemPhase::Deformation};
		outputCommit.Resources = {
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write}};
		outputCommit.Prerequisites = {GameWorldSystemIds::SkinningMatrixEvaluation, GameWorldSystemIds::MorphOutputCommit};
		outputCommit.Execution = Ranges(SingleItemGrain);
		graph.Add(std::move(outputCommit));

		GameSystemDesc transform{GameWorldSystemIds::TransformEvaluation, "Game.TransformEvaluation", GameSystemPhase::Transform};
		transform.DeclareQuery<TransformEvaluationQuery>();
		transform.Resources = {{GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Read}};
		transform.Execution = Ranges(TransformGrain);
		graph.Add(std::move(transform));

		GameSystemDesc cameraDerived{GameWorldSystemIds::CameraDerivedState, "Game.CameraDerivedState", GameSystemPhase::Transform};
		cameraDerived.DeclareQuery<CameraDerivedStateQuery>();
		cameraDerived.Prerequisites = {GameWorldSystemIds::TransformEvaluation};
		cameraDerived.Execution = Ranges(TransformGrain);
		graph.Add(std::move(cameraDerived));

		GameSystemDesc extraction{GameWorldSystemIds::MeshExtraction, "Game.MeshExtraction", GameSystemPhase::Extraction};
		extraction.DeclareQuery<MeshExtractionQuery>();
		extraction.Resources = {
		    {GameSystemResourceDomain::MeshResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Write}};
		extraction.Execution = Ranges(ExtractionGrain);
		graph.Add(std::move(extraction));

		GameSystemDesc extractionCommit{GameWorldSystemIds::ExtractionCommit, "Game.ExtractionCommit", GameSystemPhase::Extraction};
		extractionCommit.Resources = {
		    {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::ExtractionOutput, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::WorldPublication, GameSystemAccessMode::Write}};
		extractionCommit.Prerequisites = {GameWorldSystemIds::MeshExtraction};
		extractionCommit.Execution = Ranges(SingleItemGrain);
		graph.Add(std::move(extractionCommit));
		return graph.Compile();
	}

	bool ExecuteGameWorldSystems(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    TaskExecutor& executor,
	    const CameraInputIntent& cameraIntent,
	    float deltaSeconds)
	{
		if (!state.m_systemGraph || !state.m_animationOutput.Prepare(
		                                state.m_registry,
		                                resources.AnimationClips,
		                                resources.Skeletons,
		                                state.m_morphWeights,
		                                resources.Generation) ||
		    !state.m_extraction.Prepare(state.m_registry))
			return false;

		state.m_systemArena.CameraChanges.assign(StorageCount<Camera>(state.m_registry), EntityId::Invalid());
		state.m_systemArena.MotionChanges.assign(StorageCount<OscillatingMotion>(state.m_registry), EntityId::Invalid());
		state.m_systemArena.AnimationChanges.assign(StorageCount<AnimationState>(state.m_registry), EntityId::Invalid());
		state.m_systemArena.MorphChanges.assign(state.m_animationOutput.GetMorphBindings().size(), EntityId::Invalid());
		state.m_systemArena.DirtyTransforms.clear();
		if (state.m_evaluateAllTransforms)
		{
			const ComponentStorage<LocalTransform>* transforms = state.m_registry.FindStorage<LocalTransform>();
			if (transforms != nullptr)
				state.m_systemArena.DirtyTransforms.assign(transforms->GetEntities().begin(), transforms->GetEntities().end());
		}
		else
		{
			state.m_systemArena.DirtyTransforms.assign(state.m_dirtyTransforms.begin(), state.m_dirtyTransforms.end());
		}
		state.m_systemArena.DirtyTransforms.reserve(
		    state.m_systemArena.DirtyTransforms.size() + state.m_systemArena.CameraChanges.size() + state.m_systemArena.MotionChanges.size());

		StructureFrozenEpoch epoch = state.m_registry.FreezeStructure();
		if (!epoch.IsValid())
			return false;
		GameWorldSystemRun run(state, resources, cameraIntent, deltaSeconds, epoch);
		GameSystemGraphError error;
		const bool executed = state.m_systemGraph.Execute(executor, run.GetBindings(), error);
		if (executed)
		{
			state.m_dirtyTransforms.clear();
			state.m_evaluateAllTransforms = false;
		}
		return executed;
	}
}
