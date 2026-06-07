#include "PCH.h"

#include "Gltf/GltfGeometryImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Gltf/GltfMeshGeometryExtractor.h"
#include "Gltf/GltfMeshInstancingImporter.h"
#include "Gltf/GltfNodeTransformUtils.h"
#include "Gltf/GltfSkinImporter.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
#include <utility>

std::size_t GltfGeometryImporter::CountImportedMeshInstances(const cgltf_data* data)
{
	std::size_t totalPrimitives = 0;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.mesh)
		{
			std::size_t meshInstanceCount = 1;
			if (node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0)
			{
				const cgltf_accessor* firstAccessor = node.mesh_gpu_instancing.attributes[0].data;
				meshInstanceCount = firstAccessor != nullptr ? firstAccessor->count : 1;
			}

			totalPrimitives += node.mesh->primitives_count * meshInstanceCount;
		}
	}

	return totalPrimitives;
}

std::string GltfGeometryImporter::BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex)
{
	const std::string nodeName = node.name ? node.name : std::string("<unnamed-node>");
	return std::format("node '{}' primitive {}", nodeName, primitiveIndex);
}

void GltfGeometryImporter::ImportGeometry(const cgltf_data* data, SourceImportResult& result)
{
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (!node.mesh)
		{
			continue;
		}

		const DirectX::XMMATRIX worldTransform = GltfNodeTransformUtils::ComputeNodeWorldTransform(&node);
		GltfMeshGpuInstancingTransforms meshGpuInstancingTransforms;
		const std::string nodeLabel = node.name ? node.name : std::format("node {}", nodeIndex);
		const bool hasMeshGpuInstancing = node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0;
		const bool importMeshGpuInstancing =
		    hasMeshGpuInstancing &&
		    GltfMeshInstancingImporter::TryReadMeshGpuInstancingTransforms(node, nodeLabel, result, meshGpuInstancingTransforms);
		const ImportedSkeletonIndex skeletonIndex =
		    node.skin != nullptr ? GltfSkinImporter::ImportSkeleton(data, node.skin, result) : kInvalidImportedSkeletonIndex;

		for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
			const std::string primitiveLabel = BuildPrimitiveLabel(node, primitiveIndex);
			const std::uint32_t sourceMeshIndex = static_cast<std::uint32_t>(cgltf_mesh_index(data, node.mesh));
			const std::uint32_t sourcePrimitiveIndex = static_cast<std::uint32_t>(primitiveIndex);

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				GltfImportDiagnosticLog::ReportSkippedNonTrianglePrimitive(primitiveLabel, result);
				continue;
			}

			if (primitive.targets_count > 0)
			{
				GltfImportDiagnosticLog::ReportIgnoredMorphTargets(primitiveLabel, result);
			}

			if (primitive.has_draco_mesh_compression)
			{
				GltfImportDiagnosticLog::ReportSkippedDracoPrimitive(primitiveLabel, result);
				continue;
			}

			if (primitive.mappings_count > 0)
			{
				GltfImportDiagnosticLog::ReportIgnoredMaterialVariantMappings(primitiveLabel, result);
			}

			ImportedMeshPrimitiveIndex importedPrimitiveIndex = FindImportedPrimitiveIndex(result.scene, sourceMeshIndex, sourcePrimitiveIndex);
			if (importedPrimitiveIndex == kInvalidImportedMeshPrimitiveIndex)
			{
				ImportedMeshGeometry meshGeometry = GltfMeshGeometryExtractor::ExtractMeshGeometry(primitive);
				if (!meshGeometry.IsValid())
				{
					GltfImportDiagnosticLog::ReportSkippedIncompletePrimitive(primitiveLabel, result);
					continue;
				}

				ImportedMeshPrimitive primitiveEntry;
				primitiveEntry.geometry = std::move(meshGeometry);
				primitiveEntry.displayName = primitiveLabel;
				primitiveEntry.sourceMeshIndex = sourceMeshIndex;
				primitiveEntry.sourcePrimitiveIndex = sourcePrimitiveIndex;
				importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(result.scene.meshPrimitives.size());
				result.scene.meshPrimitives.push_back(std::move(primitiveEntry));
			}

			const ImportedMaterialIndex materialIndex = ResolveMaterialIndex(primitive, data, primitiveLabel, result);
			if (importMeshGpuInstancing)
			{
				AppendMeshGpuInstancingGroup(
				    result,
				    meshGpuInstancingTransforms,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    skeletonIndex,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view());
			}
			else
			{
				AppendMeshInstance(
				    result,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    kInvalidImportedMeshInstanceGroupIndex,
				    skeletonIndex,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view());
			}
		}
	}
}

ImportedMeshPrimitiveIndex GltfGeometryImporter::FindImportedPrimitiveIndex(
	const ImportedScene& scene,
	std::uint32_t sourceMeshIndex,
	std::uint32_t sourcePrimitiveIndex) noexcept
{
	for (std::size_t primitiveIndex = 0; primitiveIndex < scene.meshPrimitives.size(); ++primitiveIndex)
	{
		const ImportedMeshPrimitive& primitive = scene.meshPrimitives[primitiveIndex];
		if (primitive.sourceMeshIndex == sourceMeshIndex && primitive.sourcePrimitiveIndex == sourcePrimitiveIndex)
		{
			return static_cast<ImportedMeshPrimitiveIndex>(primitiveIndex);
		}
	}

	return kInvalidImportedMeshPrimitiveIndex;
}

void GltfGeometryImporter::AppendMeshInstance(
    SourceImportResult& result,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX worldTransform,
    ImportedMeshInstanceGroupIndex groupIndex,
    ImportedSkeletonIndex skeletonIndex,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName)
{
	ImportedMeshInstance instanceEntry;
	instanceEntry.primitiveIndex = importedPrimitiveIndex;
	instanceEntry.materialIndex = materialIndex;
	instanceEntry.groupIndex = groupIndex;
	instanceEntry.skeletonIndex = skeletonIndex;
	DirectX::XMStoreFloat4x4(&instanceEntry.worldTransform, worldTransform);
	instanceEntry.sourceNodeIndex = sourceNodeIndex;
	instanceEntry.sourceNodeName = sourceNodeName;
	result.scene.meshInstances.push_back(std::move(instanceEntry));
}

void GltfGeometryImporter::AppendMeshGpuInstancingGroup(
    SourceImportResult& result,
    const GltfMeshGpuInstancingTransforms& transforms,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX nodeWorldTransform,
    ImportedSkeletonIndex skeletonIndex,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName)
{
	const ImportedMeshInstanceGroupIndex groupIndex = static_cast<ImportedMeshInstanceGroupIndex>(result.scene.meshInstanceGroups.size());
	const ImportedMeshInstanceIndex firstInstanceIndex = static_cast<ImportedMeshInstanceIndex>(result.scene.meshInstances.size());

	for (std::size_t instanceIndex = 0; instanceIndex < transforms.instanceCount; ++instanceIndex)
	{
		const DirectX::XMMATRIX authoredInstanceTransform =
		    GltfMeshInstancingImporter::BuildMeshGpuInstancingTransform(transforms, instanceIndex);
		const DirectX::XMMATRIX worldTransform = DirectX::XMMatrixMultiply(nodeWorldTransform, authoredInstanceTransform);
		AppendMeshInstance(
		    result,
		    importedPrimitiveIndex,
		    materialIndex,
		    worldTransform,
		    groupIndex,
		    skeletonIndex,
		    sourceNodeIndex,
		    sourceNodeName);
	}

	ImportedMeshInstanceGroup groupEntry;
	groupEntry.primitiveIndex = importedPrimitiveIndex;
	groupEntry.materialIndex = materialIndex;
	groupEntry.firstInstanceIndex = firstInstanceIndex;
	groupEntry.instanceCount = static_cast<std::uint32_t>(transforms.instanceCount);
	groupEntry.groupKind = ImportedMeshInstanceGroupKind::AuthoredInstanceGroup;
	result.scene.meshInstanceGroups.push_back(groupEntry);
}

ImportedMaterialIndex GltfGeometryImporter::ResolveMaterialIndex(
	const cgltf_primitive& primitive,
	const cgltf_data* data,
	std::string_view primitiveLabel,
	SourceImportResult& result)
{
	if (!primitive.material || result.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	const std::uint32_t materialIndex = static_cast<std::uint32_t>(primitive.material - data->materials);
	if (materialIndex < result.scene.materials.size())
	{
		return materialIndex;
	}

	GltfImportDiagnosticLog::ReportInvalidMaterialIndex(primitiveLabel, materialIndex, result);
	return kInvalidImportedMaterialIndex;
}
