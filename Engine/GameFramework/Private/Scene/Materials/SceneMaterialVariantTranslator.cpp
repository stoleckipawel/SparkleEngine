#include "PCH.h"

#include "Scene/Materials/SceneMaterialVariantTranslator.h"

#include "World/GameWorldState.h"

#include <utility>

namespace SceneMaterialVariantTranslator
{
	namespace
	{
		void AppendBindingsForMeshAsset(
		    const SceneAssetPayload& sceneAssetPayload,
		    const SceneAssetPayload::MaterialVariantMapping& mapping,
		    MaterialHandle materialBaseHandle,
		    SceneMeshInstanceIndex sceneMeshBaseIndex,
		    const ECS::GameWorldState& world,
		    std::vector<MaterialVariantBinding>& outBindings)
		{
			if (!mapping.material.IsValid() || !materialBaseHandle.IsValid())
				return;

			const MaterialHandle sceneMaterialHandle(
			    materialBaseHandle.GetIndex() + mapping.material.GetIndex(), materialBaseHandle.GetGeneration());
			SceneMeshInstanceIndex localMeshInstanceIndex = 0;
			if (mapping.meshAssetKind == Assets::CookedMeshAssetKind::Static)
			{
				for (const SceneAssetPayload::StaticMeshInstance& meshInstance : sceneAssetPayload.staticMeshInstances)
				{
					if (meshInstance.meshAssetIndex == mapping.meshAssetIndex)
					{
						outBindings.push_back(MaterialVariantBinding{
						    .Variant = mapping.variantIndex,
						    .Entity = world.GetMeshEntity(sceneMeshBaseIndex + localMeshInstanceIndex),
						    .Material = sceneMaterialHandle});
					}
					++localMeshInstanceIndex;
				}
				return;
			}

			localMeshInstanceIndex = static_cast<SceneMeshInstanceIndex>(sceneAssetPayload.staticMeshInstances.size());
			for (const SceneAssetPayload::SkeletalMeshInstance& meshInstance : sceneAssetPayload.skeletalMeshInstances)
			{
				if (meshInstance.meshAssetIndex == mapping.meshAssetIndex)
				{
					outBindings.push_back(MaterialVariantBinding{
					    .Variant = mapping.variantIndex,
					    .Entity = world.GetMeshEntity(sceneMeshBaseIndex + localMeshInstanceIndex),
					    .Material = sceneMaterialHandle});
				}
				++localMeshInstanceIndex;
			}
		}
	}

	std::vector<MaterialVariantDesc> BuildDescriptions(const SceneAssetPayload& sceneAssetPayload)
	{
		std::vector<MaterialVariantDesc> variants;
		variants.reserve(sceneAssetPayload.materialVariants.size());
		for (const SceneAssetPayload::MaterialVariant& payloadVariant : sceneAssetPayload.materialVariants)
		{
			MaterialVariantDesc variant;
			variant.name = payloadVariant.name;
			variant.sourceVariantIndex = payloadVariant.sourceVariantIndex;
			variants.push_back(std::move(variant));
		}
		return variants;
	}

	std::vector<MaterialVariantBinding> BuildBindings(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex,
	    const ECS::GameWorldState& world)
	{
		std::vector<MaterialVariantBinding> bindings;
		bindings.reserve(sceneAssetPayload.materialVariantMappings.size());
		for (const SceneAssetPayload::MaterialVariantMapping& mapping : sceneAssetPayload.materialVariantMappings)
			AppendBindingsForMeshAsset(sceneAssetPayload, mapping, materialBaseHandle, sceneMeshBaseIndex, world, bindings);
		return bindings;
	}
}
