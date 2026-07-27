#include "PCH.h"

#include "Level/Loading/SceneLoadPackageBuilder.h"

#include "Level/Loading/SceneLoadExecutionState.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/Components/WorldComponentSchemas.h"

#include <cstdint>
#include <format>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

class SceneLoadPackageAssembly final
{
  public:
	static std::uint64_t MakeAuthoredInstanceId(std::string_view identity) noexcept
	{
		constexpr std::uint64_t OffsetBasis = 14695981039346656037ull;
		constexpr std::uint64_t Prime = 1099511628211ull;
		std::uint64_t hash = OffsetBasis;
		for (char character : identity)
		{
			hash ^= static_cast<std::uint8_t>(character);
			hash *= Prime;
		}
		return hash == 0 ? 1 : hash;
	}

	template <typename... Components>
	static std::vector<ECS::ComponentSchema> Schemas() noexcept
	{
		return {ECS::GetComponentSchema<Components>()...};
	}

	static std::size_t EstimatePayloadBytes(const SceneAssetPayload& payload) noexcept
	{
		return payload.staticMeshAssets.size() * sizeof(SceneAssetPayload::StaticMeshAsset) +
		       payload.skeletalMeshAssets.size() * sizeof(SceneAssetPayload::SkeletalMeshAsset) +
		       payload.staticMeshInstances.size() * sizeof(SceneAssetPayload::StaticMeshInstance) +
		       payload.skeletalMeshInstances.size() * sizeof(SceneAssetPayload::SkeletalMeshInstance) +
		       payload.meshInstanceGroups.size() * sizeof(SceneAssetPayload::MeshInstanceGroup) +
		       payload.materials.size() * sizeof(MaterialDesc) + payload.cameras.size() * sizeof(SceneAssetPayload::Camera) +
		       payload.lights.size() * sizeof(SceneLightDesc) + payload.skeletons.size() * sizeof(SkeletonResource) +
		       payload.animations.size() * sizeof(AnimationClipResource);
	}

	static bool ValidateReferences(const SceneAssetPayload& payload, std::string& errorMessage)
	{
		auto materialIsValid = [&payload](MaterialHandle handle)
		{
			return !handle.IsValid() || handle.GetIndex() < payload.materials.size();
		};
		for (const SceneAssetPayload::StaticMeshInstance& instance : payload.staticMeshInstances)
		{
			if (instance.meshAssetIndex >= payload.staticMeshAssets.size() || !materialIsValid(instance.material))
			{
				errorMessage = "Static mesh instance references an invalid mesh or material.";
				return false;
			}
		}
		for (const SceneAssetPayload::SkeletalMeshInstance& instance : payload.skeletalMeshInstances)
		{
			if (instance.meshAssetIndex >= payload.skeletalMeshAssets.size() ||
			    instance.skeletonAssetId == Assets::InvalidCookedAssetId || !materialIsValid(instance.material))
			{
				errorMessage = "Skeletal mesh instance has an invalid mesh, material, or skeleton reference.";
				return false;
			}
		}
		for (const SceneAssetPayload::MaterialVariantMapping& mapping : payload.materialVariantMappings)
		{
			if (!materialIsValid(mapping.material) || mapping.variantIndex >= payload.materialVariants.size())
			{
				errorMessage = "Material variant contains an invalid material or variant reference.";
				return false;
			}
		}
		return true;
	}

	static void BuildBlueprints(Assets::SceneAssetLoadWork& work)
	{
		auto add = [&work](std::string identity, std::vector<ECS::ComponentSchema> schemas)
		{
			work.Entities.push_back(Assets::EntityBlueprint{std::move(identity), std::move(schemas)});
		};
		for (const AnimationClipResource& animation : work.Payload.animations)
			add(
			    std::format("{}:animation:{}", work.Id.value, animation.sourceAnimationIndex),
			    Schemas<ECS::AnimationState, ECS::Name, ECS::AuthoredIdentity, ECS::EditorMetadata>());
		for (std::size_t index = 0; index < work.Payload.staticMeshInstances.size(); ++index)
		{
			const SceneAssetPayload::StaticMeshInstance& instance = work.Payload.staticMeshInstances[index];
			add(
			    std::format("{}:mesh:{}:{}", work.Id.value, instance.sourceNodeIndex, index),
			    Schemas<ECS::LocalTransform, ECS::WorldTransform, ECS::MeshInstance, ECS::Visibility, ECS::AuthoredIdentity,
			            ECS::EditorMetadata>());
		}
		for (std::size_t index = 0; index < work.Payload.skeletalMeshInstances.size(); ++index)
		{
			const SceneAssetPayload::SkeletalMeshInstance& instance = work.Payload.skeletalMeshInstances[index];
			add(
			    std::format("{}:skinned-mesh:{}:{}", work.Id.value, instance.sourceNodeIndex, index),
			    Schemas<ECS::LocalTransform, ECS::WorldTransform, ECS::MeshInstance, ECS::Visibility, ECS::MorphState,
			            ECS::SkinningState, ECS::AuthoredIdentity, ECS::EditorMetadata>());
		}
		for (std::size_t index = 0; index < work.Payload.cameras.size(); ++index)
			add(
			    std::format("{}:camera:{}", work.Id.value, index),
			    Schemas<ECS::LocalTransform, ECS::WorldTransform, ECS::Camera, ECS::CameraDerivedState, ECS::Visibility,
			            ECS::CameraMovement, ECS::Name, ECS::AuthoredIdentity, ECS::EditorMetadata>());
		for (std::size_t index = 0; index < work.Payload.lights.size(); ++index)
			add(
			    std::format("{}:light:{}", work.Id.value, index),
			    Schemas<ECS::LocalTransform, ECS::WorldTransform, ECS::Light, ECS::Visibility, ECS::Name, ECS::AuthoredIdentity,
			            ECS::EditorMetadata>());
	}

	static bool HasUniqueBlueprintContract(
	    const std::vector<Assets::EntityBlueprint>& entities,
	    std::string& errorMessage)
	{
		std::unordered_set<std::string> identities;
		for (const Assets::EntityBlueprint& entity : entities)
		{
			if (entity.AuthoredIdentity.empty())
			{
				errorMessage = "Scene load package contains an empty authored entity identity.";
				return false;
			}
			if (!identities.insert(entity.AuthoredIdentity).second)
			{
				errorMessage = std::format("Scene load package contains duplicate authored entity identity '{}'.", entity.AuthoredIdentity);
				return false;
			}
			if (entity.Components.empty())
			{
				errorMessage = std::format("Scene load package entity '{}' has no component contract.", entity.AuthoredIdentity);
				return false;
			}
			std::unordered_set<std::uint64_t> componentIds;
			for (const ECS::ComponentSchema component : entity.Components)
			{
				if (!component.Id.IsValid() || component.Version == 0 || !componentIds.insert(component.Id.Value).second)
				{
					errorMessage = "Scene load package contains an invalid or duplicate component schema record.";
					return false;
				}
			}
		}
		return true;
	}
};

namespace Assets
{
	bool SceneLoadPackageBuilder::BuildAssetBlueprints(
	    SceneAssetLoadWork& work,
	    std::size_t& decodedBytes,
	    std::string& errorMessage)
	{
		if (!SceneLoadPackageAssembly::ValidateReferences(work.Payload, errorMessage))
			return false;
		work.Payload.authoredInstanceId = SceneLoadPackageAssembly::MakeAuthoredInstanceId(work.Id.value);
		SceneLoadPackageAssembly::BuildBlueprints(work);
		decodedBytes = SceneLoadPackageAssembly::EstimatePayloadBytes(work.Payload);
		errorMessage.clear();
		return true;
	}

	bool SceneLoadPackageBuilder::Finalize(SceneLoadSharedState& state, std::string& errorMessage)
	{
		std::unordered_map<std::uint64_t, std::string_view> instanceIdentities;
		for (const SceneAssetLoadWork& work : state.Assets)
		{
			const auto [existing, inserted] = instanceIdentities.emplace(work.Payload.authoredInstanceId, work.Id.value);
			if (work.Payload.authoredInstanceId == 0 || (!inserted && existing->second != work.Id.value))
			{
				errorMessage = "Scene load package contains an invalid or colliding authored instance identity.";
				return false;
			}
		}
		std::unordered_set<Assets::CookedAssetId> skeletonAssets;
		for (const SceneAssetLoadWork& work : state.Assets)
			for (const SkeletonResource& skeleton : work.Payload.skeletons)
				skeletonAssets.insert(skeleton.assetId);
		for (const SceneAssetLoadWork& work : state.Assets)
			for (const SceneAssetPayload::SkeletalMeshInstance& mesh : work.Payload.skeletalMeshInstances)
				if (!skeletonAssets.contains(mesh.skeletonAssetId))
				{
					errorMessage = "Scene load package contains an unresolved cross-asset skeleton reference.";
					return false;
				}

		state.Package->Entities.push_back(EntityBlueprint{
		    std::format("level:{}:camera:0", state.Package->Level.name),
		    SceneLoadPackageAssembly::Schemas<ECS::LocalTransform, ECS::WorldTransform, ECS::Camera, ECS::CameraDerivedState, ECS::Visibility,
		            ECS::CameraMovement, ECS::Name, ECS::AuthoredIdentity, ECS::EditorMetadata>()});
		for (std::size_t index = 0; index < state.Package->Level.lights.size(); ++index)
			state.Package->Entities.push_back(EntityBlueprint{
			    std::format("level:{}:light:{}", state.Package->Level.name, index),
			    SceneLoadPackageAssembly::Schemas<
			        ECS::LocalTransform,
			        ECS::WorldTransform,
			        ECS::Light,
			        ECS::Visibility,
			        ECS::Name,
			        ECS::AuthoredIdentity,
			            ECS::EditorMetadata>()});
		for (SceneAssetLoadWork& work : state.Assets)
		{
			for (EntityBlueprint& entity : work.Entities)
				state.Package->Entities.push_back(std::move(entity));
			state.Package->AssetPayloads.push_back(std::move(work.Payload));
		}
		return SceneLoadPackageAssembly::HasUniqueBlueprintContract(state.Package->Entities, errorMessage);
	}
}
