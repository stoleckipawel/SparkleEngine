#include "PCH.h"
#include "World/GameWorldState.h"

#include "Scene/Animations/SceneAnimationDiagnostics.h"
#include "Scene/Animations/SceneAnimationPoseEvaluator.h"
#include "Scene/Animations/SceneMorphWeightEvaluator.h"
#include "Scene/Skeletons/SceneSkeletons.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/EditorComponents.h"

#include <algorithm>
#include <cmath>

namespace ECS
{
	void GameWorldState::AppendAnimationClips(std::vector<SceneAnimationClipDesc>&& clips)
	{
		for (SceneAnimationClipDesc& clip : clips)
		{
			const std::uint32_t unsupportedCount = SceneAnimationDiagnostics::CountUnsupportedRuntimeChannels(clip);
			SceneAnimationDiagnostics::LogLoadedClip(clip);
			if (unsupportedCount > 0u)
			{
				SceneAnimationDiagnostics::LogUnsupportedRuntimeChannels(clip, unsupportedCount);
			}
			const std::string name = clip.name;
			const std::uint32_t sourceAnimationIndex = clip.sourceAnimationIndex;
			const Assets::CookedAssetId animationAssetId = clip.animationAssetId;
			const AnimationResourceHandle resource = m_animationResources.Add(std::move(clip));
			if (!resource.IsValid())
			{
				continue;
			}
			const EntityId entity = m_registry.Create();
			if (!entity.IsValid())
			{
				continue;
			}
			const bool added = m_registry.Add(
			                       entity,
			                       AnimationState{.Resource = resource, .AnimationAssetId = animationAssetId}) &&
			                   m_registry.Add(entity, Name{name}) &&
			                   m_registry.Add(
			                       entity,
			                       AuthoredIdentity{
			                           .SourceAssetId = animationAssetId,
			                           .SourceObjectId = sourceAnimationIndex,
			                           .Kind = AuthoredObjectKind::Animation}) &&
			                   m_registry.Add(entity, EditorMetadata{});
			if (!added)
			{
				m_registry.Destroy(entity);
				continue;
			}
			RecordChange(entity, WorldChangeKind::EntityCreated, WorldDataKind::World);
			RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::AnimationState);
		}
	}

	void GameWorldState::UpdateAnimations(float deltaSeconds, const SceneSkeletons& skeletons)
	{
		SceneAnimationSnapshot output;
		const ComponentStorage<AnimationState>* animations = m_registry.FindStorage<AnimationState>();
		if (animations == nullptr)
		{
			m_animationResources.SetDerivedOutput(std::move(output));
			return;
		}
		const std::span<const EntityId> entities = animations->GetEntities();
		const std::span<const AnimationState> states = animations->GetComponents();
		for (std::size_t index = 0; index < states.size(); ++index)
		{
			AnimationState state = states[index];
			const SceneAnimationClipDesc* clip = m_animationResources.Resolve(state.Resource);
			if (clip == nullptr)
			{
				continue;
			}
			if (state.Playing && clip->durationSeconds > 0.0f)
			{
				state.TimeSeconds += (std::max)(0.0f, deltaSeconds) * state.PlaybackRate;
				state.TimeSeconds = state.Looping ? std::fmod(state.TimeSeconds, clip->durationSeconds)
				                                  : std::min(state.TimeSeconds, clip->durationSeconds);
				m_registry.Replace(entities[index], state);
				RecordChange(entities[index], WorldChangeKind::ValueChanged, WorldDataKind::AnimationState);
			}
			SceneAnimationPoseEvaluator::AppendMatchingPose(*clip, state.TimeSeconds, skeletons, output.poses);
			SceneMorphWeightEvaluator::AppendSnapshots(*clip, state.TimeSeconds, output.morphWeights);
		}
		m_animationResources.SetDerivedOutput(std::move(output));
	}
}
