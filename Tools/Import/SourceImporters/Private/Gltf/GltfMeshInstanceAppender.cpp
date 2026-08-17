#include "PCH.h"

#include "Gltf/GltfMeshInstanceAppender.h"

#include <utility>

void GltfMeshInstanceAppender::AppendMeshInstance(
    SourceImportOutput& output,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX worldTransform,
    ImportedMeshInstanceGroupIndex groupIndex,
    ImportedSkeletonIndex skeletonIndex,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName,
    std::span<const float> morphWeights)
{
	ImportedMeshInstance instanceEntry;
	instanceEntry.primitiveIndex = importedPrimitiveIndex;
	instanceEntry.materialIndex = materialIndex;
	instanceEntry.groupIndex = groupIndex;
	instanceEntry.skeletonIndex = skeletonIndex;
	DirectX::XMStoreFloat4x4(&instanceEntry.worldTransform, worldTransform);
	instanceEntry.sourceNodeIndex = sourceNodeIndex;
	instanceEntry.sourceNodeName = sourceNodeName;
	instanceEntry.morphWeights.assign(morphWeights.begin(), morphWeights.end());
	output.scene.meshInstances.push_back(std::move(instanceEntry));
}

void GltfMeshInstanceAppender::AppendMeshGpuInstancingGroup(
    SourceImportOutput& output,
    const GltfMeshGpuInstancingTransforms& transforms,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX nodeWorldTransform,
    ImportedSkeletonIndex skeletonIndex,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName,
    std::span<const float> morphWeights)
{
	const ImportedMeshInstanceGroupIndex groupIndex = static_cast<ImportedMeshInstanceGroupIndex>(output.scene.meshInstanceGroups.size());
	const ImportedMeshInstanceIndex firstInstanceIndex = static_cast<ImportedMeshInstanceIndex>(output.scene.meshInstances.size());

	for (std::size_t instanceIndex = 0; instanceIndex < transforms.instanceCount; ++instanceIndex)
	{
		const DirectX::XMMATRIX authoredInstanceTransform =
		    GltfMeshInstancingImporter::BuildMeshGpuInstancingTransform(transforms, instanceIndex);
		const DirectX::XMMATRIX worldTransform = DirectX::XMMatrixMultiply(authoredInstanceTransform, nodeWorldTransform);
		AppendMeshInstance(
		    output,
		    importedPrimitiveIndex,
		    materialIndex,
		    worldTransform,
		    groupIndex,
		    skeletonIndex,
		    sourceNodeIndex,
		    sourceNodeName,
		    morphWeights);
	}

	ImportedMeshInstanceGroup groupEntry;
	groupEntry.primitiveIndex = importedPrimitiveIndex;
	groupEntry.materialIndex = materialIndex;
	groupEntry.firstInstanceIndex = firstInstanceIndex;
	groupEntry.instanceCount = static_cast<std::uint32_t>(transforms.instanceCount);
	groupEntry.groupKind = ImportedMeshInstanceGroupKind::AuthoredInstanceGroup;
	output.scene.meshInstanceGroups.push_back(groupEntry);
}
