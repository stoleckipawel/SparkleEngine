#include "PCH.h"

#include "Scene/Materials/SceneMaterialVariantPayloadApplier.h"

#include <utility>

namespace SceneMaterialVariantPayloadApplier
{
	namespace
	{
		void AppendBindingsForMeshAsset(
		    const SceneAssetPayload& sceneAssetPayload,
		    const SceneAssetPayload::MaterialVariantMapping& mapping,
		    MaterialHandle materialBaseHandle,
		    SceneMeshInstanceIndex sceneMeshBaseIndex,
		    std::vector<SceneMaterialVariantBinding>& outBindings)
		{
			if (!mapping.material.IsValid() || !materialBaseHandle.IsValid())
			{
				return;
			}

			const MaterialHandle sceneMaterialHandle(materialBaseHandle.GetIndex() + mapping.material.GetIndex());
			SceneMeshInstanceIndex localMeshInstanceIndex = 0;
			if (mapping.meshAssetKind == Assets::CookedMeshAssetKind::Static)
			{
				for (const SceneAssetPayload::StaticMeshInstance& meshInstance : sceneAssetPayload.staticMeshInstances)
				{
					if (meshInstance.meshAssetIndex == mapping.meshAssetIndex)
					{
						outBindings.push_back(
						    SceneMaterialVariantBinding{
						        .variantIndex = mapping.variantIndex,
						        .meshInstanceIndex = sceneMeshBaseIndex + localMeshInstanceIndex,
						        .material = sceneMaterialHandle});
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
					outBindings.push_back(
					    SceneMaterialVariantBinding{
					        .variantIndex = mapping.variantIndex,
					        .meshInstanceIndex = sceneMeshBaseIndex + localMeshInstanceIndex,
					        .material = sceneMaterialHandle});
				}
				++localMeshInstanceIndex;
			}
		}
	}  // namespace

	std::vector<SceneMaterialVariantDesc> BuildVariantDescs(const SceneAssetPayload& sceneAssetPayload)
	{
		std::vector<SceneMaterialVariantDesc> variants;
		variants.reserve(sceneAssetPayload.materialVariants.size());
		for (const SceneAssetPayload::MaterialVariant& payloadVariant : sceneAssetPayload.materialVariants)
		{
			SceneMaterialVariantDesc variant;
			variant.name = payloadVariant.name;
			variant.sourceVariantIndex = payloadVariant.sourceVariantIndex;
			variants.push_back(std::move(variant));
		}

		return variants;
	}

	std::vector<SceneMaterialVariantBinding> BuildVariantBindings(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex)
	{
		std::vector<SceneMaterialVariantBinding> bindings;
		bindings.reserve(sceneAssetPayload.materialVariantMappings.size());
		for (const SceneAssetPayload::MaterialVariantMapping& mapping : sceneAssetPayload.materialVariantMappings)
		{
			AppendBindingsForMeshAsset(sceneAssetPayload, mapping, materialBaseHandle, sceneMeshBaseIndex, bindings);
		}

		return bindings;
	}
}
