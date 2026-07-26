#include "PCH.h"
#include "World/Extraction/Dynamic/RenderFrameDynamicDataExtractor.h"

#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/ComponentStorage.h"
#include "World/Extraction/Identity/RenderObjectIdentityMap.h"
#include "World/Extraction/Structural/RenderObjectDeltaExtractor.h"
#include "World/Extraction/WorldExtractionStorage.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/WorldReadView.h"

#include <limits>
#include <map>
#include <utility>

namespace ECS
{
	struct RenderMorphMetadata final
	{
		RenderAnimationAssetHandle Animation;
		std::uint32_t TargetNodeIndex =
		    (std::numeric_limits<std::uint32_t>::max)();
	};

	void RenderFrameDynamicDataExtractor::Extract(
	    GameWorldState& state,
	    const GameWorldResourceStores& resources,
	    const WorldReadView& readView,
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    const RenderObjectDeltaExtractor& objects,
	    RenderObjectIdentityMap& identities,
	    RenderFrameDynamicData& dynamic) const
	{
		dynamic.Camera = BuildCamera(readView);
		ExtractObjects(meshes, objects, dynamic);
		ExtractSkinning(state, resources, meshes, objects, dynamic);
		ExtractMorphWeights(state, objects, dynamic);
		ExtractLights(readView, identities, dynamic);
	}

	void RenderFrameDynamicDataExtractor::ExtractObjects(
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    const RenderObjectDeltaExtractor& objects,
	    RenderFrameDynamicData& dynamic)
	{
		dynamic.Objects.reserve(meshes.size());
		for (const WorldExtractionStorage::MeshSlot& mesh : meshes)
			dynamic.Objects.push_back(
			    {objects.FindObject(mesh.Entity), mesh.WorldMatrix, mesh.WorldInverseTranspose, mesh.Visible});
	}

	void RenderFrameDynamicDataExtractor::ExtractSkinning(
	    GameWorldState& state,
	    const GameWorldResourceStores& resources,
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    const RenderObjectDeltaExtractor& objects,
	    RenderFrameDynamicData& dynamic)
	{
		const AnimationOutput& output = state.m_animationOutput.GetOutput();
		for (const WorldExtractionStorage::MeshSlot& mesh : meshes)
		{
			const SkinningState* skinning = state.m_registry.Get<SkinningState>(mesh.Entity);
			if (skinning == nullptr || !skinning->Pose.IsValid() ||
			    skinning->Pose.Generation != state.m_animationOutput.GetTargetGeneration() ||
			    skinning->Pose.Slot >= output.poses.size())
				continue;
			const AnimationPoseOutput& pose = output.poses[skinning->Pose.Slot];
			const SkeletonResourceHandle skeleton = resources.Skeletons.Find(pose.skeletonAssetId);
			const AnimationState* animation = state.m_registry.Get<AnimationState>(pose.animationEntity);
			dynamic.Skinning.push_back(
			    {objects.FindObject(mesh.Entity),
			     skeleton.IsValid() ? RenderSkeletonAssetHandle(pose.skeletonAssetId)
			                        : RenderSkeletonAssetHandle{},
			     animation != nullptr
			         ? RenderAnimationAssetHandle(pose.animationAssetId)
			         : RenderAnimationAssetHandle{},
			     pose.skinningMatrices});
		}

	}

	void RenderFrameDynamicDataExtractor::ExtractMorphWeights(
	    GameWorldState& state,
	    const RenderObjectDeltaExtractor& objects,
	    RenderFrameDynamicData& dynamic)
	{
		const AnimationOutput& output = state.m_animationOutput.GetOutput();
		const auto samples = state.m_animationOutput.GetMorphSamples();
		std::map<EntityId, RenderMorphMetadata> metadata;
		for (const AnimationOutputStorage::MorphTargetBinding& binding : state.m_animationOutput.GetMorphBindings())
		{
			if (binding.SampleIndex >= samples.size()) continue;
			const std::uint32_t outputIndex = samples[binding.SampleIndex].OutputIndex;
			if (outputIndex >= output.morphWeights.size()) continue;
			const MorphWeightOutput& morph = output.morphWeights[outputIndex];
			const AnimationState* animation = state.m_registry.Get<AnimationState>(morph.animationEntity);
			metadata.insert_or_assign(
			    binding.TargetEntity,
			    RenderMorphMetadata{
			        .Animation =
			            animation != nullptr
			                ? RenderAnimationAssetHandle(
			                      animation->AnimationAssetId)
			                : RenderAnimationAssetHandle{},
			        .TargetNodeIndex =
			            morph.targetNodeIndex});
		}

		const ComponentStorage<MorphState>* morphStates =
		    state.m_registry.FindStorage<MorphState>();
		if (morphStates == nullptr)
		{
			return;
		}

		std::map<RenderObjectId, RenderMorphData> ordered;
		const std::span<const EntityId> entities =
		    morphStates->GetEntities();
		const std::span<const MorphState> components =
		    morphStates->GetComponents();
		for (std::size_t index = 0;
		     index < entities.size();
		     ++index)
		{
			const RenderObjectId object =
			    objects.FindObject(entities[index]);
			const std::span<const float> weights =
			    state.m_morphWeights.Read(
			        components[index].Weights);
			if (!object.IsValid() || weights.empty())
			{
				continue;
			}

			const auto animationMetadata =
			    metadata.find(entities[index]);
			ordered.emplace(
			    object,
			    RenderMorphData{
			        .Object = object,
			        .Animation =
			            animationMetadata != metadata.end()
			                ? animationMetadata->second.Animation
			                : RenderAnimationAssetHandle{},
			        .TargetNodeIndex =
			            animationMetadata != metadata.end()
			                ? animationMetadata->second
			                      .TargetNodeIndex
			                : (std::numeric_limits<
			                      std::uint32_t>::max)(),
			        .Weights =
			            std::vector<float>(
			                weights.begin(),
			                weights.end())});
		}
		dynamic.MorphWeights.reserve(
		    dynamic.MorphWeights.size() + ordered.size());
		for (auto& entry : ordered)
		{
			dynamic.MorphWeights.push_back(
			    std::move(entry.second));
		}
	}

	void RenderFrameDynamicDataExtractor::ExtractLights(
	    const WorldReadView& readView,
	    RenderObjectIdentityMap& identities,
	    RenderFrameDynamicData& dynamic)
	{
		std::map<EntityId, const SceneLightDesc*> ordered;
		for (const WorldLightReadData& light : readView.GetLights())
			ordered.emplace(light.Entity, &light.Description);
		dynamic.Lights.reserve(ordered.size());
		for (const auto& [entity, description] : ordered)
			dynamic.Lights.push_back({identities.Resolve(entity), *description});
	}

	RenderCameraData RenderFrameDynamicDataExtractor::BuildCamera(const WorldReadView& readView) noexcept
	{
		for (const WorldCameraReadData& camera : readView.GetCameras())
		{
			if (!camera.Active) continue;
			return {.Position = camera.LocalTransform.GetTranslation(), .Direction = camera.Direction,
			        .FovYDegrees = camera.Description.fovYDegrees, .AspectRatio = camera.AspectRatio,
			        .NearZ = camera.Description.nearZ, .FarZ = camera.Description.farZ};
		}
		return {};
	}
}
