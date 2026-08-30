#include "PCH.h"

#include "AnimationSystemExecution.h"

#include "Animation/AnimationOutputStorage.h"
#include "Animation/AnimationPoseEvaluator.h"
#include "Animation/MorphWeightEvaluator.h"
#include "Animation/SkinningMatrixEvaluator.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <algorithm>
#include <cmath>

namespace ECS
{
	AnimationSystemExecution::AnimationSystemExecution(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    float deltaSeconds,
	    const StructureFrozenEpoch& epoch) :
	    m_state(state),
	    m_resources(resources),
	    m_deltaSeconds((std::max) (0.0f, deltaSeconds)),
	    m_playbackQuery(state.m_registry, epoch),
	    m_poseQuery(state.m_registry, epoch),
	    m_morphQuery(state.m_registry, epoch)
	{
		m_playbackQuery.PrepareWriteTraversal();
	}

	std::uint32_t AnimationSystemExecution::GetPlaybackCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_playbackQuery.GetEstimatedEntityCount());
	}

	std::uint32_t AnimationSystemExecution::GetPoseCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_poseQuery.GetEstimatedEntityCount());
	}

	std::uint32_t AnimationSystemExecution::GetMorphSampleCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_state.m_animationOutput.GetMorphSamples().size());
	}

	std::uint32_t AnimationSystemExecution::GetSkinningCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_state.m_animationOutput.GetPoseWork().size());
	}

	bool AnimationSystemExecution::RunPlayback(std::uint32_t begin, std::uint32_t end)
	{
		return m_playbackQuery
		    .ForEachRange(
		        begin,
		        end,
		        [this](std::size_t index, EntityId entity, AnimationState& state)
		        {
			        const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(state.Resource);
			        if (!clip.IsValid() || !state.Playing || clip.Resource->durationSeconds <= 0.0f)
				        return;
			        state.TimeSeconds += m_deltaSeconds * state.PlaybackRate;
			        state.TimeSeconds = state.Looping ? std::fmod(state.TimeSeconds, clip.Resource->durationSeconds)
			                                          : (std::min) (state.TimeSeconds, clip.Resource->durationSeconds);
			        m_state.m_systemArena.AnimationChanges[index] = entity;
		        })
		    .Succeeded();
	}

	bool AnimationSystemExecution::RunPose(std::uint32_t begin, std::uint32_t end)
	{
		return m_poseQuery
		    .ForEachRange(
		        begin,
		        end,
		        [this](std::size_t, EntityId entity, const AnimationState& state)
		        {
			        AnimationOutputStorage::PoseWorkSlot* work = m_state.m_animationOutput.FindPoseWork(entity);
			        if (work == nullptr || work->PoseOutputIndex >= m_state.m_animationOutput.GetMutableOutput().poses.size())
				        return;
			        const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(state.Resource);
			        const SkeletonEvaluationData skeleton = m_resources.Skeletons.Resolve(work->Skeleton);
			        if (!clip.IsValid()
			            || !AnimationPoseEvaluator::Evaluate(
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

	bool AnimationSystemExecution::RunMorphSamples(std::uint32_t begin, std::uint32_t end)
	{
		std::span<AnimationOutputStorage::MorphSampleSlot> samples = m_state.m_animationOutput.GetMorphSamples();
		AnimationOutput& output = m_state.m_animationOutput.GetMutableOutput();
		return m_morphQuery
		    .ForEachEntityRange(
		        m_state.m_animationOutput.GetMorphEntities(),
		        begin,
		        end,
		        [this, samples, &output](std::size_t index, EntityId, const AnimationState& state)
		        {
			        const AnimationOutputStorage::MorphSampleSlot& sample = samples[index];
			        const ResolvedAnimationClip clip = m_resources.AnimationClips.Resolve(sample.Clip);
			        if (!clip.IsValid() || sample.OutputIndex >= output.morphWeights.size())
				        return;
			        MorphWeightEvaluator::Evaluate(
			            *clip.Resource,
			            sample.ChannelIndex,
			            state.TimeSeconds,
			            output.morphWeights[sample.OutputIndex].weights);
		        })
		    .Succeeded();
	}

	bool AnimationSystemExecution::RunSkinning(std::uint32_t begin, std::uint32_t end)
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
			        output.poses[slot.PoseOutputIndex].jointMatrices))
				return false;
		}
		return true;
	}

	bool AnimationSystemExecution::CommitMorphOutputs(std::uint32_t, std::uint32_t)
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
			if (outputIndex >= output.morphWeights.size()
			    || !m_state.m_morphWeights.Write(binding.TargetWeights, output.morphWeights[outputIndex].weights))
				return false;
			m_state.m_systemArena.MorphChanges[index] = binding.TargetEntity;
		}
		return true;
	}
}
