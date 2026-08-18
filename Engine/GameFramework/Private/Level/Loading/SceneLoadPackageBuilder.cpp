#include "PCH.h"

#include "Level/Loading/SceneLoadPackageBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Level/Loading/SceneLoadWorkState.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/ECS/Components/WorldComponentSchemas.h"

#include <cstdint>
#include <format>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

static const auto g_sceneLoadPackageBuilderLogger = Logging::GetOrCreateLogger("GameFramework.SceneLoadPackageBuilder");

class SceneLoadPackageAssembly final
{
public:
	static std::uint64_t MakeAuthoredInstanceId(std::string_view identity)
	{
		constexpr std::uint64_t OffsetBasis = 14695981039346656037ull;
		constexpr std::uint64_t Prime = 1099511628211ull;
		std::uint64_t hash = OffsetBasis;
		for (char character : identity)
		{
			hash ^= static_cast<std::uint8_t>(character);
			hash *= Prime;
		}
		if (hash == 0)
			throw Diagnostics::Error(std::format("Scene asset identity '{}' hashes to the reserved null identity.", identity));
		return hash;
	}

	template <typename... Components> static std::vector<ECS::ComponentSchema> Schemas() noexcept
	{
		return {ECS::GetComponentSchema<Components>()...};
	}

	static void AddCount(std::size_t& total, std::size_t amount, std::string_view description)
	{
		if (amount > (std::numeric_limits<std::size_t>::max)() - total)
			throw Diagnostics::Error(std::format("Scene load package {} count exceeds the host address range.", description));
		total += amount;
	}

	static void ReserveFinalPackageStorage(Assets::SceneLoadWorkState& state)
	{
		std::size_t entityCount = state.Package->Entities.size();
		AddCount(entityCount, 1u, "entity");
		AddCount(entityCount, state.Package->Level.lights.size(), "entity");
		for (const Assets::SceneAssetLoadWork& work : state.Assets)
			AddCount(entityCount, work.Entities.size(), "entity");
		std::size_t payloadCount = state.Package->AssetPayloads.size();
		AddCount(payloadCount, state.Assets.size(), "payload");
		state.Package->Entities.reserve(entityCount);
		state.Package->AssetPayloads.reserve(payloadCount);
	}

	static void BuildBlueprints(Assets::SceneAssetLoadWork& work)
	{
		auto add = [&work](std::string identity, std::vector<ECS::ComponentSchema> schemas)
		{
			work.Entities.push_back(Assets::EntityBlueprint{std::move(identity), std::move(schemas)});
		};
		for (const AnimationClipResource& animation : work.Payload.animations)
			add(std::format("{}:animation:{}", work.Id.value, animation.sourceAnimationIndex),
			    Schemas<ECS::AnimationState, ECS::Name, ECS::AuthoredIdentity, ECS::EditorMetadata>());
		for (std::size_t index = 0; index < work.Payload.staticMeshInstances.size(); ++index)
		{
			const SceneAssetPayload::StaticMeshInstance& instance = work.Payload.staticMeshInstances[index];
			add(std::format("{}:mesh:{}:{}", work.Id.value, instance.sourceNodeIndex, index),
			    Schemas<
			        ECS::LocalTransform,
			        ECS::WorldTransform,
			        ECS::MeshInstance,
			        ECS::Visibility,
			        ECS::AuthoredIdentity,
			        ECS::EditorMetadata>());
		}
		for (std::size_t index = 0; index < work.Payload.skeletalMeshInstances.size(); ++index)
		{
			const SceneAssetPayload::SkeletalMeshInstance& instance = work.Payload.skeletalMeshInstances[index];
			add(std::format("{}:skinned-mesh:{}:{}", work.Id.value, instance.sourceNodeIndex, index),
			    Schemas<
			        ECS::LocalTransform,
			        ECS::WorldTransform,
			        ECS::MeshInstance,
			        ECS::Visibility,
			        ECS::MorphState,
			        ECS::SkinningState,
			        ECS::AuthoredIdentity,
			        ECS::EditorMetadata>());
		}
		for (std::size_t index = 0; index < work.Payload.cameras.size(); ++index)
			add(std::format("{}:camera:{}", work.Id.value, index),
			    Schemas<
			        ECS::LocalTransform,
			        ECS::WorldTransform,
			        ECS::Camera,
			        ECS::CameraDerivedState,
			        ECS::Visibility,
			        ECS::Name,
			        ECS::AuthoredIdentity,
			        ECS::EditorMetadata>());
		for (std::size_t index = 0; index < work.Payload.lights.size(); ++index)
			add(std::format("{}:light:{}", work.Id.value, index),
			    Schemas<
			        ECS::LocalTransform,
			        ECS::WorldTransform,
			        ECS::Light,
			        ECS::Visibility,
			        ECS::Name,
			        ECS::AuthoredIdentity,
			        ECS::EditorMetadata>());
	}

	static void ValidateBlueprintContract(const std::vector<Assets::EntityBlueprint>& entities)
	{
		std::unordered_set<std::string> identities;
		for (const Assets::EntityBlueprint& entity : entities)
		{
			if (entity.AuthoredIdentity.empty())
			{
				Diagnostics::Fatal(
				    g_sceneLoadPackageBuilderLogger,
				    __FILE__,
				    __LINE__,
				    "Scene package assembly produced an empty authored entity identity.");
			}
			if (!identities.insert(entity.AuthoredIdentity).second)
			{
				throw Diagnostics::Error(
				    std::format("Scene load package contains duplicate authored entity identity '{}'.", entity.AuthoredIdentity));
			}
			if (entity.Components.empty())
			{
				Diagnostics::Fatal(
				    g_sceneLoadPackageBuilderLogger,
				    __FILE__,
				    __LINE__,
				    std::format("Scene package entity '{}' has no component contract.", entity.AuthoredIdentity));
			}
			std::unordered_set<std::uint64_t> componentIds;
			for (const ECS::ComponentSchema component : entity.Components)
			{
				if (!component.Id.IsValid() || !componentIds.insert(component.Id.Value).second)
				{
					Diagnostics::Fatal(
					    g_sceneLoadPackageBuilderLogger,
					    __FILE__,
					    __LINE__,
					    "Scene package assembly produced an invalid or duplicate component schema record.");
				}
			}
		}
	}
};

namespace Assets
{
	void SceneLoadPackageBuilder::BuildAssetBlueprints(SceneAssetLoadWork& work)
	{
		work.Payload.authoredInstanceId = SceneLoadPackageAssembly::MakeAuthoredInstanceId(work.Id.value);
		SceneLoadPackageAssembly::BuildBlueprints(work);
	}

	void SceneLoadPackageBuilder::Finalize(SceneLoadWorkState& state)
	{
		SceneLoadPackageAssembly::ReserveFinalPackageStorage(state);
		std::unordered_map<std::uint64_t, std::string_view> instanceIdentities;
		for (const SceneAssetLoadWork& work : state.Assets)
		{
			const auto [existing, inserted] = instanceIdentities.emplace(work.Payload.authoredInstanceId, work.Id.value);
			if (work.Payload.authoredInstanceId == 0 || (!inserted && existing->second != work.Id.value))
			{
				throw Diagnostics::Error("Scene load package contains an invalid or colliding authored instance identity.");
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
					throw Diagnostics::Error("Scene load package contains an unresolved cross-asset skeleton reference.");
				}

		state.Package->Entities.push_back(
		    EntityBlueprint{
		        std::format("level:{}:camera:0", state.Package->Level.name),
		        SceneLoadPackageAssembly::Schemas<
		            ECS::LocalTransform,
		            ECS::WorldTransform,
			            ECS::Camera,
			            ECS::CameraDerivedState,
			            ECS::Visibility,
			            ECS::Name,
		            ECS::AuthoredIdentity,
		            ECS::EditorMetadata>()});
		for (std::size_t index = 0; index < state.Package->Level.lights.size(); ++index)
			state.Package->Entities.push_back(
			    EntityBlueprint{
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
			std::vector<EntityBlueprint>().swap(work.Entities);
			state.Package->AssetPayloads.push_back(std::move(work.Payload));
		}
		SceneLoadPackageAssembly::ValidateBlueprintContract(state.Package->Entities);
	}
}
