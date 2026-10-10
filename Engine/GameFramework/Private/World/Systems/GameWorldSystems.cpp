#include "PCH.h"

#include "World/Systems/GameWorldSystems.h"

#include "Tasks/Public/TaskExecutor.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/Systems/Descriptors/GameWorldSystemContract.h"
#include "World/Systems/Execution/SystemChangeCommitter.h"
#include "World/Systems/GameSystemGraph.h"

namespace ECS
{

	template <typename T> std::uint32_t StorageCount(const EntityRegistry& registry) noexcept
	{
		const ComponentStorage<T>* storage = registry.FindStorage<T>();
		return storage == nullptr ? 0u : static_cast<std::uint32_t>(storage->GetEntities().size());
	}

	GameWorldSystemExecution::GameWorldSystemExecution(GameWorldState& state, const GameWorldSystemExecutionContext& context, const StructureFrozenEpoch& epoch) :
	    m_simulation(state, context.Camera, epoch),
	    m_animation(state, context.Resources, context.Camera.DeltaSeconds, epoch),
	    m_transform(state, epoch),
	    m_extraction(state, context.Resources.Skeletons, epoch),
	    m_state(state)
	{
	}

	CompiledGameSystemGraph GameWorldSystemExecution::BuildGraph()
	{
		GameSystemGraph graph;
		GameSystemDesc camera{"Game.CameraMovement", GameSystemPhase::Simulation};
		camera.DeclareQuery<CameraMovementQuery>();

		camera.Resources = {
		    {GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::CameraInputIntent, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::CameraNavigationSettings, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Write}};

		camera.RangePolicy = GameWorldSystemGrain::Camera;

		camera.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_simulation.GetCameraCount();
		};

		camera.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_simulation.RunCamera(begin, end);
		};

		graph.Add(std::move(camera));

		GameSystemDesc playback{"Game.AnimationPlaybackAdvance", GameSystemPhase::Animation};
		playback.DeclareQuery<PlaybackAdvanceQuery>();

		playback.Resources = {
		    {GameSystemResourceDomain::UpdateInputs, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Write}};

		playback.RangePolicy = GameWorldSystemGrain::Animation;

		playback.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_animation.GetPlaybackCount();
		};

		playback.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_animation.RunPlayback(begin, end);
		};

		const auto playbackIndex = graph.Add(std::move(playback));

		GameSystemDesc pose{"Game.AnimationPoseEvaluation", GameSystemPhase::Animation};
		pose.DeclareQuery<PoseEvaluationQuery>();

		pose.Resources = {
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Write}};

		pose.Prerequisites = {playbackIndex};
		pose.RangePolicy = GameWorldSystemGrain::Pose;

		pose.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_animation.GetPoseCount();
		};

		pose.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_animation.RunPose(begin, end);
		};

		const auto poseIndex = graph.Add(std::move(pose));

		GameSystemDesc morph{"Game.MorphWeightEvaluation", GameSystemPhase::Animation};
		morph.DeclareQuery<MorphEvaluationQuery>();
		morph.Resources = {{GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read}, {GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Write}};
		morph.Prerequisites = {playbackIndex};
		morph.RangePolicy = GameWorldSystemGrain::Animation;

		morph.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_animation.GetMorphSampleCount();
		};

		morph.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_animation.RunMorphSamples(begin, end);
		};

		const auto morphIndex = graph.Add(std::move(morph));

		GameSystemDesc skinning{"Game.SkinningMatrixEvaluation", GameSystemPhase::Deformation};

		skinning.Resources = {
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Write}};

		skinning.Prerequisites = {poseIndex};
		skinning.RangePolicy = GameWorldSystemGrain::Pose;

		skinning.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_animation.GetSkinningCount();
		};

		skinning.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_animation.RunSkinning(begin, end);
		};

		const auto skinningIndex = graph.Add(std::move(skinning));

		GameSystemDesc morphCommit{"Game.MorphOutputCommit", GameSystemPhase::Deformation};
		morphCommit.Resources = {{GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Read}, {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Write}};
		morphCommit.Prerequisites = {morphIndex};
		morphCommit.RangePolicy = GameWorldSystemGrain::SingleItem;

		morphCommit.GetItemCount = [](GameWorldSystemExecution&)
		{
			return 1u;
		};

		morphCommit.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_animation.CommitMorphOutputs(begin, end);
		};

		const auto morphCommitIndex = graph.Add(std::move(morphCommit));

		GameSystemDesc outputCommit{"Game.SystemOutputCommit", GameSystemPhase::Deformation};

		outputCommit.Resources = {
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SystemChangeScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write}};

		outputCommit.Prerequisites = {skinningIndex, morphCommitIndex};
		outputCommit.RangePolicy = GameWorldSystemGrain::SingleItem;

		outputCommit.GetItemCount = [](GameWorldSystemExecution&)
		{
			return 1u;
		};

		outputCommit.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t, std::uint32_t)
		{
			return SystemChangeCommitter::CommitSystemOutputs(self.m_state);
		};

		graph.Add(std::move(outputCommit));

		GameSystemDesc transform{"Game.TransformEvaluation", GameSystemPhase::Transform};
		transform.DeclareQuery<TransformEvaluationQuery>();
		transform.Resources = {{GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Read}, {GameSystemResourceDomain::TransformScratch, GameSystemAccessMode::Write}};
		transform.RangePolicy = GameWorldSystemGrain::Transform;

		transform.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_transform.GetDirtyTransformCount();
		};

		transform.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_transform.RunTransforms(begin, end);
		};

		const auto transformIndex = graph.Add(std::move(transform));

		GameSystemDesc cameraDerived{"Game.CameraDerivedState", GameSystemPhase::Transform};
		cameraDerived.DeclareQuery<CameraDerivedStateQuery>();
		cameraDerived.Resources = {{GameSystemResourceDomain::DirtyTransforms, GameSystemAccessMode::Read}, {GameSystemResourceDomain::CameraDerivedScratch, GameSystemAccessMode::Write}};
		cameraDerived.Prerequisites = {transformIndex};
		cameraDerived.RangePolicy = GameWorldSystemGrain::Transform;

		cameraDerived.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_transform.GetDirtyTransformCount();
		};

		cameraDerived.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_transform.RunCameraDerivedState(begin, end);
		};

		graph.Add(std::move(cameraDerived));

		GameSystemDesc extraction{"Game.MeshExtraction", GameSystemPhase::Extraction};
		extraction.DeclareQuery<MeshExtractionQuery>();

		extraction.Resources = {
		    {GameSystemResourceDomain::MeshResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphOutput, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Write}};

		extraction.RangePolicy = GameWorldSystemGrain::Extraction;

		extraction.GetItemCount = [](GameWorldSystemExecution& self)
		{
			return self.m_extraction.GetMeshCount();
		};

		extraction.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t begin, std::uint32_t end)
		{
			return self.m_extraction.Run(begin, end);
		};

		const auto extractionIndex = graph.Add(std::move(extraction));

		GameSystemDesc extractionCommit{"Game.ExtractionCommit", GameSystemPhase::Extraction};

		extractionCommit.Resources = {
		    {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::ExtractionOutput, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::TransformScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::CameraDerivedScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::WorldChanges, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::WorldPublication, GameSystemAccessMode::Write}};

		extractionCommit.Prerequisites = {extractionIndex};
		extractionCommit.RangePolicy = GameWorldSystemGrain::SingleItem;

		extractionCommit.GetItemCount = [](GameWorldSystemExecution&)
		{
			return 1u;
		};

		extractionCommit.ExecuteRange = [](GameWorldSystemExecution& self, std::uint32_t, std::uint32_t)
		{
			return SystemChangeCommitter::CommitExtraction(self.m_state);
		};

		graph.Add(std::move(extractionCommit));
		return graph.Compile();
	}

	CompiledGameSystemGraph BuildGameWorldSystemGraph()
	{
		return GameWorldSystemExecution::BuildGraph();
	}

	bool ExecuteGameWorldSystems(GameWorldState& state, const GameWorldSystemExecutionContext& context)
	{
		if (!state.m_systemGraph
		    || !state.m_animationOutput.Prepare(state.m_registry, context.Resources.AnimationClips, context.Resources.Skeletons, state.m_morphWeights, context.Resources.Generation)
		    || !state.m_extraction.Prepare(state.m_registry))
			return false;

		state.m_systemArena.CameraChanges.assign(StorageCount<Camera>(state.m_registry), EntityId::Invalid());
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
		state.m_systemArena.DirtyTransforms.reserve(state.m_systemArena.DirtyTransforms.size() + state.m_systemArena.CameraChanges.size());
		const std::size_t maximumDirtyCount = state.m_systemArena.DirtyTransforms.size() + state.m_systemArena.CameraChanges.size();
		state.m_systemArena.EvaluatedTransforms.assign(maximumDirtyCount, EntityId::Invalid());
		state.m_systemArena.CameraDerivedChanges.assign(maximumDirtyCount, EntityId::Invalid());

		StructureFrozenEpoch epoch = state.m_registry.FreezeStructure();
		if (!epoch.IsValid())
			return false;
		GameWorldSystemExecution systems(state, context, epoch);
		GameSystemGraphError error;
		const bool executed = state.m_systemGraph.Execute(context.Executor, systems, error);
		if (executed)
		{
			state.m_dirtyTransforms.clear();
			state.m_evaluateAllTransforms = false;
		}
		return executed;
	}
}
