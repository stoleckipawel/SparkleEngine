#include "PCH.h"
#include "World/GameWorldState.h"

#include "Animation/AnimationDiagnostics.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/Systems/GameWorldSystems.h"

#include <utility>

namespace ECS
{
	void GameWorldState::AppendAnimationClips(
	    std::vector<AnimationClipResource> clips,
	    AnimationClipResourceStore& resources,
	    std::uint64_t sourceInstanceId)
	{
		for (AnimationClipResource& clip : clips)
		{
			const std::uint32_t unsupportedCount = AnimationDiagnostics::CountUnsupportedRuntimeChannels(clip);
			AnimationDiagnostics::LogLoadedClip(clip);
			if (unsupportedCount > 0u)
				AnimationDiagnostics::LogUnsupportedRuntimeChannels(clip, unsupportedCount);
			const std::string name = clip.name;
			const std::uint32_t sourceAnimationIndex = clip.sourceAnimationIndex;
			const Assets::CookedAssetId animationAssetId = clip.animationAssetId;
			const AnimationResourceHandle resource = resources.Add(std::move(clip));
			if (!resource.IsValid())
			{
				continue;
			}
			const EntityId entity = m_registry.Create();
			if (!entity.IsValid())
			{
				continue;
			}
			const bool added = m_registry.Add(entity, AnimationState{.Resource = resource, .AnimationAssetId = animationAssetId})
			    && m_registry.Add(entity, Name{name})
			    && m_registry.Add(
			        entity,
			        AuthoredIdentity{
			            .SourceAssetId = animationAssetId,
			            .SourceInstanceId = sourceInstanceId,
			            .SourceObjectId = sourceAnimationIndex,
			            .Kind = AuthoredObjectKind::Animation})
			    && m_registry.Add(entity, EditorMetadata{});
			if (!added)
			{
				m_registry.Destroy(entity);
				continue;
			}
			RecordChange(entity, WorldChangeKind::EntityCreated, WorldDataKind::World);
			RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::AnimationState);
		}
	}

	bool GameWorldState::PrepareSystemResources(GameWorldResourceStores& resources)
	{
		return resources.AnimationClips.ResolveTargets(resources.Skeletons, resources.Generation)
		    && m_animationOutput.Prepare(m_registry, resources.AnimationClips, resources.Skeletons, m_morphWeights, resources.Generation)
		    && m_extraction.Prepare(m_registry);
	}

	bool GameWorldState::ExecuteSystems(const GameWorldSystemExecutionContext& context)
	{
		return ExecuteGameWorldSystems(*this, context);
	}
}
