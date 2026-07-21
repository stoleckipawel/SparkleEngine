#include "PCH.h"

#include "World/Systems/GameWorldSystems.h"

#include "Tasks/Public/TaskExecutor.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/MotionComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/Systems/Descriptors/GameWorldSystemContract.h"
#include "World/Systems/Execution/AnimationSystemExecution.h"
#include "World/Systems/Execution/MeshExtractionSystemExecution.h"
#include "World/Systems/Execution/SimulationSystemExecution.h"
#include "World/Systems/Execution/SystemChangeCommitter.h"
#include "World/Systems/Execution/TransformSystemExecution.h"
#include "World/Systems/GameSystemGraph.h"

#include <array>

namespace ECS
{
	namespace
	{
		template <typename T> std::uint32_t StorageCount(const EntityRegistry& registry) noexcept
		{
			const ComponentStorage<T>* storage = registry.FindStorage<T>();
			return storage == nullptr ? 0u : static_cast<std::uint32_t>(storage->GetEntities().size());
		}
	}

	class GameWorldSystemBindingSet final
	{
	  public:
		GameWorldSystemBindingSet(
		    GameWorldState& state,
		    GameWorldResourceStores& resources,
		    const CameraInputIntent& cameraIntent,
		    float deltaSeconds,
		    const StructureFrozenEpoch& epoch) :
		    m_simulation(state, cameraIntent, deltaSeconds, epoch),
		    m_animation(state, resources, deltaSeconds, epoch),
		    m_transform(state, epoch),
		    m_extraction(state, epoch),
		    m_state(state)
		{
			BuildBindings();
		}

		std::span<const GameSystemExecutionBinding> GetBindings() const noexcept { return m_bindings; }

	  private:
		void BuildBindings()
		{
			m_bindings = {{
			    {GameWorldSystemIds::CameraMovement, [this] { return m_simulation.GetCameraCount(); }, [this](auto b, auto e) { return m_simulation.RunCamera(b, e); }},
			    {GameWorldSystemIds::OscillatingMeshMotion, [this] { return m_simulation.GetMotionCount(); }, [this](auto b, auto e) { return m_simulation.RunMotion(b, e); }},
			    {GameWorldSystemIds::PlaybackAdvance, [this] { return m_animation.GetPlaybackCount(); }, [this](auto b, auto e) { return m_animation.RunPlayback(b, e); }},
			    {GameWorldSystemIds::PoseEvaluation, [this] { return m_animation.GetPoseCount(); }, [this](auto b, auto e) { return m_animation.RunPose(b, e); }},
			    {GameWorldSystemIds::MorphWeightEvaluation, [this] { return m_animation.GetMorphSampleCount(); }, [this](auto b, auto e) { return m_animation.RunMorphSamples(b, e); }},
			    {GameWorldSystemIds::SkinningMatrixEvaluation, [this] { return m_animation.GetSkinningCount(); }, [this](auto b, auto e) { return m_animation.RunSkinning(b, e); }},
			    {GameWorldSystemIds::MorphOutputCommit, [] { return 1u; }, [this](auto b, auto e) { return m_animation.CommitMorphOutputs(b, e); }},
			    {GameWorldSystemIds::SystemOutputCommit, [] { return 1u; }, [this](auto, auto) { return SystemChangeCommitter::CommitSystemOutputs(m_state, m_simulation.GetNextMotionTime()); }},
			    {GameWorldSystemIds::TransformEvaluation, [this] { return m_transform.GetDirtyTransformCount(); }, [this](auto b, auto e) { return m_transform.RunTransforms(b, e); }},
			    {GameWorldSystemIds::CameraDerivedState, [this] { return m_transform.GetDirtyTransformCount(); }, [this](auto b, auto e) { return m_transform.RunCameraDerivedState(b, e); }},
			    {GameWorldSystemIds::MeshExtraction, [this] { return m_extraction.GetMeshCount(); }, [this](auto b, auto e) { return m_extraction.Run(b, e); }},
			    {GameWorldSystemIds::ExtractionCommit, [] { return 1u; }, [this](auto, auto) { return SystemChangeCommitter::CommitExtraction(m_state); }},
			}};
		}

		SimulationSystemExecution m_simulation;
		AnimationSystemExecution m_animation;
		TransformSystemExecution m_transform;
		MeshExtractionSystemExecution m_extraction;
		GameWorldState& m_state;
		std::array<GameSystemExecutionBinding, 12> m_bindings;
	};

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
		const std::size_t maximumDirtyCount = state.m_systemArena.DirtyTransforms.size() +
		                                      state.m_systemArena.CameraChanges.size() +
		                                      state.m_systemArena.MotionChanges.size();
		state.m_systemArena.EvaluatedTransforms.assign(maximumDirtyCount, EntityId::Invalid());
		state.m_systemArena.CameraDerivedChanges.assign(maximumDirtyCount, EntityId::Invalid());

		StructureFrozenEpoch epoch = state.m_registry.FreezeStructure();
		if (!epoch.IsValid()) return false;
		GameWorldSystemBindingSet bindings(state, resources, cameraIntent, deltaSeconds, epoch);
		GameSystemGraphError error;
		const bool executed = state.m_systemGraph.Execute(executor, bindings.GetBindings(), error);
		if (executed)
		{
			state.m_dirtyTransforms.clear();
			state.m_evaluateAllTransforms = false;
		}
		return executed;
	}
}
