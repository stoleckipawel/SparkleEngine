#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMaterialVariantAppender.h"

#include <format>
#include <utility>

namespace Assets
{
	void SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(
	    const LoadedSceneManifest& sceneManifest,
	    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
	    SceneAssetPayload& sceneAssetPayload)
	{
		const auto variantBaseIndex = static_cast<MaterialVariantIndex>(sceneAssetPayload.materialVariants.size());
		sceneAssetPayload.materialVariants.reserve(sceneAssetPayload.materialVariants.size() + sceneManifest.materialVariants.size());
		for (const CookedSceneMaterialVariantRecord& variantRecord : sceneManifest.materialVariants)
		{
			SceneAssetPayload::MaterialVariant variant;
			variant.name = variantRecord.name;
			variant.sourceVariantIndex = variantRecord.sourceVariantIndex;
			sceneAssetPayload.materialVariants.push_back(std::move(variant));
		}

		sceneAssetPayload.materialVariantMappings.reserve(
		    sceneAssetPayload.materialVariantMappings.size() + sceneManifest.materialVariantMappings.size());
		for (const CookedSceneMaterialVariantMappingRecord& mappingRecord : sceneManifest.materialVariantMappings)
		{
			SceneAssetPayload::MaterialVariantMapping mapping;
			const SceneAssetPayloadMeshBinding& meshBinding = meshAssetBindings[mappingRecord.meshAssetIndex];
			mapping.meshAssetIndex = meshBinding.payloadMeshAssetIndex;
			mapping.meshAssetKind = meshBinding.kind;
			mapping.variantIndex = variantBaseIndex + mappingRecord.variantIndex;
			mapping.material = MaterialHandle(mappingRecord.materialAssetIndex);
			sceneAssetPayload.materialVariantMappings.push_back(mapping);
		}
	}
}
