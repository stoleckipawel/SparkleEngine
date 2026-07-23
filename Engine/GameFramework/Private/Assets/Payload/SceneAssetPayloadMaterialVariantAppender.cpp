#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMaterialVariantAppender.h"

#include <cstddef>
#include <format>
#include <utility>
#include <vector>

namespace Assets
{

		struct PayloadMeshAssetBinding
		{
			CookedMeshAssetKind kind = CookedMeshAssetKind::Static;
			SceneMeshAssetIndex payloadMeshAssetIndex = kInvalidSceneMeshAssetIndex;
		};

		std::string ReadVariantName(const CookedSceneMaterialVariantRecord& record)
		{
			std::size_t length = 0;
			while (length < kCookedSceneMaterialVariantNameCapacity && record.name[length] != '\0')
			{
				++length;
			}

			return std::string(record.name, length);
		}

		std::vector<PayloadMeshAssetBinding> BuildMeshAssetBindings(const LoadedSceneManifest& sceneManifest)
		{
			std::vector<PayloadMeshAssetBinding> bindings;
			bindings.reserve(sceneManifest.meshAssetReferences.size());
			SceneMeshAssetIndex staticMeshIndex = 0;
			SceneMeshAssetIndex skeletalMeshIndex = 0;
			for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
			{
				PayloadMeshAssetBinding binding;
				binding.kind = meshReference.meshAssetKind;
				binding.payloadMeshAssetIndex =
				    meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal ? skeletalMeshIndex++ : staticMeshIndex++;
				bindings.push_back(binding);
			}

			return bindings;
		}
	  // namespace

	bool SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(
	    const LoadedSceneManifest& sceneManifest,
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
		const std::vector<PayloadMeshAssetBinding> meshAssetBindings = BuildMeshAssetBindings(sceneManifest);
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
			const PayloadMeshAssetBinding& meshBinding = meshAssetBindings[mappingRecord.meshAssetIndex];
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
