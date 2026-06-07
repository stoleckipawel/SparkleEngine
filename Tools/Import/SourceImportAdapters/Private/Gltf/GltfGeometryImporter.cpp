#include "PCH.h"

#include "Gltf/GltfGeometryImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Gltf/GltfMeshGeometryExtractor.h"
#include "Gltf/GltfMeshInstanceAppender.h"
#include "Gltf/GltfMeshInstancingImporter.h"
#include "Gltf/GltfMorphTargetImporter.h"
#include "Gltf/GltfNodeTransformUtils.h"
#include "Gltf/GltfPrimitiveMaterialResolver.h"
#include "Gltf/GltfSkinImporter.h"

#include <cgltf.h>

#include <cstdint>
#include <format>

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

			if (primitive.has_draco_mesh_compression)
			{
				GltfImportDiagnosticLog::ReportSkippedDracoPrimitive(primitiveLabel, result);
				continue;
			}

			ImportedMeshPrimitiveIndex importedPrimitiveIndex = FindImportedPrimitiveIndex(result.scene, sourceMeshIndex, sourcePrimitiveIndex);
			if (importedPrimitiveIndex == kInvalidImportedMeshPrimitiveIndex)
			{
				ImportedMeshGeometry meshGeometry = GltfMeshGeometryExtractor::ExtractMeshGeometry(*node.mesh, primitive);
				if (!meshGeometry.IsValid())
				{
					GltfImportDiagnosticLog::ReportSkippedIncompletePrimitive(primitiveLabel, result);
					continue;
				}

				if (primitive.targets_count > 0 && (!meshGeometry.HasSkinInfluences() || !meshGeometry.HasMorphTargets()))
				{
					GltfImportDiagnosticLog::ReportIgnoredMorphTargets(primitiveLabel, result);
				}

				ImportedMeshPrimitive primitiveEntry;
				primitiveEntry.geometry = std::move(meshGeometry);
				primitiveEntry.displayName = primitiveLabel;
				primitiveEntry.sourceMeshIndex = sourceMeshIndex;
				primitiveEntry.sourcePrimitiveIndex = sourcePrimitiveIndex;
				importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(result.scene.meshPrimitives.size());
				result.scene.meshPrimitives.push_back(std::move(primitiveEntry));
			}

			const ImportedMaterialIndex materialIndex = GltfPrimitiveMaterialResolver::Resolve(primitive, data, primitiveLabel, result);
			std::vector<float> morphWeights;
			const ImportedMeshGeometry& importedGeometry = result.scene.meshPrimitives[importedPrimitiveIndex].geometry;
			if (importedGeometry.HasSkinInfluences() && importedGeometry.HasMorphTargets())
			{
				morphWeights = GltfMorphTargetImporter::BuildNodeMorphWeights(*node.mesh, node.weights, node.weights_count);
			}
			if (importMeshGpuInstancing)
			{
				GltfMeshInstanceAppender::AppendMeshGpuInstancingGroup(
				    result,
				    meshGpuInstancingTransforms,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    skeletonIndex,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view(),
				    morphWeights);
			}
			else
			{
				GltfMeshInstanceAppender::AppendMeshInstance(
				    result,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    kInvalidImportedMeshInstanceGroupIndex,
				    skeletonIndex,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view(),
				    morphWeights);
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
