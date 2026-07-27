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

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace ECS
{
	void RenderFrameDynamicDataExtractor::BeginScene() noexcept
	{
		m_publishedObjects.clear();
		m_currentObjects.clear();
		m_morphMetadata.clear();
	}

	void RenderFrameDynamicDataExtractor::Extract(
	    GameWorldState& state,
	    const GameWorldResourceStores& resources,
	    const WorldReadView& readView,
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    const RenderObjectDeltaExtractor& objects,
	    RenderObjectIdentityMap& identities,
	    RenderFrameDynamicData& dynamic)
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
		m_currentObjects.clear();
		m_currentObjects.reserve(meshes.size());
		for (const WorldExtractionStorage::MeshSlot& mesh : meshes)
		{
			m_currentObjects.push_back(
			    {objects.FindObject(mesh.Entity), mesh.WorldMatrix, mesh.WorldInverseTranspose, mesh.Visible});
		}

		std::sort(
		    m_currentObjects.begin(),
		    m_currentObjects.end(),
		    [](const RenderObjectDynamicData& left, const RenderObjectDynamicData& right)
		    {
			    return left.Object < right.Object;
		    });

		dynamic.Objects.reserve(m_currentObjects.size());
		std::size_t publishedIndex = 0u;
		for (const RenderObjectDynamicData& object : m_currentObjects)
		{
			while (publishedIndex < m_publishedObjects.size() &&
			       m_publishedObjects[publishedIndex].Object < object.Object)
			{
				++publishedIndex;
			}

			const bool unchanged =
			    publishedIndex < m_publishedObjects.size() &&
			    m_publishedObjects[publishedIndex].Object == object.Object &&
			    HasSameObjectData(m_publishedObjects[publishedIndex], object);
			if (!unchanged)
			{
				dynamic.Objects.push_back(object);
			}
		}

		m_publishedObjects.swap(m_currentObjects);
	}

	bool RenderFrameDynamicDataExtractor::HasSameObjectData(
	    const RenderObjectDynamicData& left,
	    const RenderObjectDynamicData& right) noexcept
	{
		return left.Visible == right.Visible &&
		       std::memcmp(&left.WorldMatrix, &right.WorldMatrix, sizeof(left.WorldMatrix)) == 0 &&
		       std::memcmp(
		           &left.WorldInverseTranspose,
		           &right.WorldInverseTranspose,
		           sizeof(left.WorldInverseTranspose)) == 0;
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
			const std::uint32_t matrixOffset =
			    static_cast<std::uint32_t>(dynamic.SkinningMatrices.size());
			dynamic.SkinningMatrices.insert(
			    dynamic.SkinningMatrices.end(),
			    pose.skinningMatrices.begin(),
			    pose.skinningMatrices.end());
			dynamic.Skinning.push_back(
			    RenderSkinningData{
			        .Object = objects.FindObject(mesh.Entity),
			        .Skeleton =
			            skeleton.IsValid()
			                ? RenderSkeletonAssetHandle(pose.skeletonAssetId)
			                : RenderSkeletonAssetHandle{},
			        .Animation =
			            animation != nullptr
			                ? RenderAnimationAssetHandle(pose.animationAssetId)
			                : RenderAnimationAssetHandle{},
			        .MatrixOffset = matrixOffset,
			        .MatrixCount =
			            static_cast<std::uint32_t>(pose.skinningMatrices.size())});
		}
		std::sort(
		    dynamic.Skinning.begin(),
		    dynamic.Skinning.end(),
		    [](const RenderSkinningData& lhs,
		       const RenderSkinningData& rhs)
		    {
			    return lhs.Object < rhs.Object;
		    });
	}

	void RenderFrameDynamicDataExtractor::ExtractMorphWeights(
	    GameWorldState& state,
	    const RenderObjectDeltaExtractor& objects,
	    RenderFrameDynamicData& dynamic)
	{
		const AnimationOutput& output = state.m_animationOutput.GetOutput();
		const auto samples = state.m_animationOutput.GetMorphSamples();

		m_morphMetadata.clear();
		m_morphMetadata.reserve(state.m_animationOutput.GetMorphBindings().size());
		for (const AnimationOutputStorage::MorphTargetBinding& binding : state.m_animationOutput.GetMorphBindings())
		{
			if (binding.SampleIndex >= samples.size()) continue;
			const std::uint32_t outputIndex = samples[binding.SampleIndex].OutputIndex;
			if (outputIndex >= output.morphWeights.size()) continue;
			const MorphWeightOutput& morph = output.morphWeights[outputIndex];
			const AnimationState* animation = state.m_registry.Get<AnimationState>(morph.animationEntity);
			m_morphMetadata.push_back(
			    MorphMetadata{
			        .Entity = binding.TargetEntity,
			        .Animation =
			            animation != nullptr
			                ? RenderAnimationAssetHandle(
			                      animation->AnimationAssetId)
			                : RenderAnimationAssetHandle{},
			        .TargetNodeIndex =
			            morph.targetNodeIndex});
		}
		std::stable_sort(
		    m_morphMetadata.begin(),
		    m_morphMetadata.end(),
		    [](const MorphMetadata& left, const MorphMetadata& right)
		    {
			    return left.Entity < right.Entity;
		    });

		const ComponentStorage<MorphState>* morphStates =
		    state.m_registry.FindStorage<MorphState>();
		if (morphStates == nullptr)
		{
			return;
		}

		const std::span<const EntityId> entities =
		    morphStates->GetEntities();
		const std::span<const MorphState> components =
		    morphStates->GetComponents();
		dynamic.MorphRanges.reserve(entities.size());
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

			const auto metadataEnd = std::upper_bound(
			    m_morphMetadata.begin(),
			    m_morphMetadata.end(),
			    entities[index],
			    [](EntityId entity, const MorphMetadata& metadata)
			    {
				    return entity < metadata.Entity;
			    });
			const MorphMetadata* metadata =
			    metadataEnd != m_morphMetadata.begin() &&
			            (metadataEnd - 1)->Entity == entities[index]
			        ? &*(metadataEnd - 1)
			        : nullptr;
			const std::uint32_t weightOffset =
			    static_cast<std::uint32_t>(dynamic.MorphWeights.size());
			dynamic.MorphWeights.insert(
			    dynamic.MorphWeights.end(),
			    weights.begin(),
			    weights.end());
			dynamic.MorphRanges.push_back(
			    RenderMorphData{
			        .Object = object,
			        .Animation =
			            metadata != nullptr
			                ? metadata->Animation
			                : RenderAnimationAssetHandle{},
			        .TargetNodeIndex =
			            metadata != nullptr
			                ? metadata->TargetNodeIndex
			                : (std::numeric_limits<
			                      std::uint32_t>::max)(),
			        .WeightOffset = weightOffset,
			        .WeightCount =
			            static_cast<std::uint32_t>(weights.size())});
		}
		std::sort(
		    dynamic.MorphRanges.begin(),
		    dynamic.MorphRanges.end(),
		    [](const RenderMorphData& left, const RenderMorphData& right)
		    {
			    return left.Object < right.Object;
		    });
	}

	void RenderFrameDynamicDataExtractor::ExtractLights(
	    const WorldReadView& readView,
	    RenderObjectIdentityMap& identities,
	    RenderFrameDynamicData& dynamic)
	{
		dynamic.Lights.reserve(readView.GetLights().size());
		for (const WorldLightReadData& light : readView.GetLights())
		{
			dynamic.Lights.push_back(
			    {identities.Resolve(light.Entity), light.Description});
		}
		std::sort(
		    dynamic.Lights.begin(),
		    dynamic.Lights.end(),
		    [](const RenderLightData& left, const RenderLightData& right)
		    {
			    return left.Object < right.Object;
		    });
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
