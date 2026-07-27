#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMaterialVariantAppender.h"

#include <cstddef>
#include <format>
#include <utility>
namespace Assets
{
	std::string ReadVariantName(const CookedSceneMaterialVariantRecord& record)
	{
		std::size_t length = 0;
		while (length < kCookedSceneMaterialVariantNameCapacity && record.name[length] != '\0')
		{
			++length;
		}

		return std::string(record.name, length);
	}

	bool SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(
	    const LoadedSceneManifest& sceneManifest,
	    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
	    SceneAssetPayload& sceneAssetPayload,
	    std::uint32_t materialBaseIndex,
	    std::string& errorMessage)
	{
		const auto variantBaseIndex = static_cast<MaterialVariantIndex>(sceneAssetPayload.materialVariants.size());
		sceneAssetPayload.materialVariants.reserve(sceneAssetPayload.materialVariants.size() + sceneManifest.materialVariants.size());
		for (const CookedSceneMaterialVariantRecord& variantRecord : sceneManifest.materialVariants)
		{
			SceneAssetPayload::MaterialVariant variant;
			variant.name = ReadVariantName(variantRecord);
			variant.sourceVariantIndex = variantRecord.sourceVariantIndex;
			sceneAssetPayload.materialVariants.push_back(std::move(variant));
		}

		sceneAssetPayload.materialVariantMappings.reserve(
		    sceneAssetPayload.materialVariantMappings.size() + sceneManifest.materialVariantMappings.size());
		for (const CookedSceneMaterialVariantMappingRecord& mappingRecord : sceneManifest.materialVariantMappings)
		{
			if (mappingRecord.variantIndex >= sceneManifest.materialVariants.size() ||
			    mappingRecord.materialAssetIndex >= sceneManifest.materialAssetReferences.size() ||
			    mappingRecord.meshAssetIndex >= meshAssetBindings.size())
			{
				errorMessage = "Cooked scene material variant mapping was not validated before payload append";
				return false;
			}

			SceneAssetPayload::MaterialVariantMapping mapping;
			const SceneAssetPayloadMeshBinding& meshBinding = meshAssetBindings[mappingRecord.meshAssetIndex];
			mapping.meshAssetIndex = meshBinding.payloadMeshAssetIndex;
			mapping.meshAssetKind = meshBinding.kind;
			mapping.variantIndex = variantBaseIndex + mappingRecord.variantIndex;
			mapping.material = MaterialHandle(materialBaseIndex + mappingRecord.materialAssetIndex);
			sceneAssetPayload.materialVariantMappings.push_back(mapping);
		}

		errorMessage.clear();
		return true;
	}
}
