#include "PCH.h"

#include "Gltf/GltfGeometryImporter.h"

#include "Gltf/GltfMeshGeometryExtractor.h"
#include "Gltf/GltfMeshInstanceAppender.h"
#include "Gltf/GltfMeshInstancingImporter.h"
#include "Gltf/GltfMorphTargetImporter.h"
#include "Gltf/GltfCoordinateConverter.h"
#include "Gltf/GltfPrimitiveMaterialResolver.h"
#include "Gltf/GltfSkinImporter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstdint>
#include <format>

struct GltfGeometryImporter::NodeImportContext final
{
	const cgltf_data& Data;
	const cgltf_node& Node;
	const GltfMeshGpuInstancingTransforms* MeshGpuInstancingTransforms = nullptr;
	DirectX::XMMATRIX WorldTransform;
	ImportedSkeletonIndex SkeletonIndex = kInvalidImportedSkeletonIndex;
	std::uint32_t NodeIndex = 0;
};

std::size_t GltfGeometryImporter::CountImportedMeshInstances(const cgltf_data* data)
{
	if (data == nullptr)
		throw Diagnostics::Error("glTF mesh instance count has no parsed scene.");

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
				if (firstAccessor == nullptr)
					throw Diagnostics::Error("glTF mesh GPU instancing has no transform accessor.");
				meshInstanceCount = firstAccessor->count;
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

void GltfGeometryImporter::ImportGeometry(const cgltf_data* data, SourceImportOutput& output)
{
	if (data == nullptr)
	{
		throw Diagnostics::Error("glTF geometry import has no parsed scene.");
	}

	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		ImportNode(*data, node, static_cast<std::uint32_t>(nodeIndex), output);
	}
}

void GltfGeometryImporter::ImportNode(const cgltf_data& data, const cgltf_node& node, std::uint32_t nodeIndex, SourceImportOutput& output)
{
	if (node.mesh == nullptr)
	{
		return;
	}

	GltfMeshGpuInstancingTransforms meshGpuInstancingTransforms;
	const std::string nodeLabel = node.name != nullptr ? node.name : std::format("node {}", nodeIndex);
	const bool hasMeshGpuInstancing = node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0;
	if (hasMeshGpuInstancing)
	{
		meshGpuInstancingTransforms = GltfMeshInstancingImporter::ReadMeshGpuInstancingTransforms(node, nodeLabel);
	}

	const ImportedSkeletonIndex skeletonIndex =
	    node.skin != nullptr ? GltfSkinImporter::ImportSkeleton(&data, node.skin, output) : kInvalidImportedSkeletonIndex;
	if (node.skin != nullptr && skeletonIndex == kInvalidImportedSkeletonIndex)
	{
		throw Diagnostics::Error(std::format("glTF node '{}' has an invalid skin.", nodeLabel));
	}

	const NodeImportContext context{
	    .Data = data,
	    .Node = node,
	    .MeshGpuInstancingTransforms = hasMeshGpuInstancing ? &meshGpuInstancingTransforms : nullptr,
	    .WorldTransform = node.skin == nullptr ? GltfCoordinateConverter::ComputeNodeWorldTransform(&node)
	                                           : GltfSkinImporter::ComputeSkinReferenceToWorldTransform(node.skin),
	    .SkeletonIndex = skeletonIndex,
	    .NodeIndex = nodeIndex};
	for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
	{
		ImportPrimitive(context, node.mesh->primitives[primitiveIndex], static_cast<std::uint32_t>(primitiveIndex), output);
	}
}

void GltfGeometryImporter::ImportPrimitive(
    const NodeImportContext& context,
    const cgltf_primitive& primitive,
    std::uint32_t primitiveIndex,
    SourceImportOutput& output)
{
	const std::string primitiveLabel = BuildPrimitiveLabel(context.Node, primitiveIndex);
	const ImportedMeshPrimitiveIndex importedPrimitiveIndex =
	    ResolveImportedPrimitive(context, primitive, primitiveIndex, primitiveLabel, output);

	const ImportedMeshGeometry& geometry = output.scene.meshPrimitives[importedPrimitiveIndex].geometry;
	ValidateDeformation(geometry, primitive, context.SkeletonIndex, primitiveLabel, output.scene);

	const ImportedMaterialIndex materialIndex = GltfPrimitiveMaterialResolver::Resolve(primitive, &context.Data, primitiveLabel, output);
	const std::vector<float> morphWeights = BuildMorphWeights(context, geometry, primitiveLabel);
	AppendInstances(context, importedPrimitiveIndex, materialIndex, morphWeights, output);
}

ImportedMeshPrimitiveIndex GltfGeometryImporter::ResolveImportedPrimitive(
    const NodeImportContext& context,
    const cgltf_primitive& primitive,
    std::uint32_t primitiveIndex,
    std::string_view primitiveLabel,
    SourceImportOutput& output)
{
	if (primitive.type != cgltf_primitive_type_triangles || primitive.has_draco_mesh_compression)
	{
		throw Diagnostics::Error(
		    std::format(
		        "glTF {} {}.",
		        primitiveLabel,
		        primitive.has_draco_mesh_compression ? "uses unsupported Draco compression" : "is not a triangle primitive"));
	}

	const std::uint32_t sourceMeshIndex = static_cast<std::uint32_t>(cgltf_mesh_index(&context.Data, context.Node.mesh));
	if (const ImportedMeshPrimitiveIndex existing = FindImportedPrimitiveIndex(output.scene, sourceMeshIndex, primitiveIndex);
	    existing != kInvalidImportedMeshPrimitiveIndex)
	{
		return existing;
	}

	ImportedMeshGeometry meshGeometry;
	try
	{
		meshGeometry = GltfMeshGeometryExtractor::ExtractMeshGeometry(*context.Node.mesh, primitive);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(std::format("glTF {} geometry import failed: {}", primitiveLabel, error.what()));
	}

	ImportedMeshPrimitive primitiveEntry;
	primitiveEntry.geometry = std::move(meshGeometry);
	primitiveEntry.displayName = primitiveLabel;
	primitiveEntry.sourceMeshIndex = sourceMeshIndex;
	primitiveEntry.sourcePrimitiveIndex = primitiveIndex;
	const ImportedMeshPrimitiveIndex importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(output.scene.meshPrimitives.size());
	output.scene.meshPrimitives.push_back(std::move(primitiveEntry));
	return importedPrimitiveIndex;
}

void GltfGeometryImporter::ValidateDeformation(
    const ImportedMeshGeometry& geometry,
    const cgltf_primitive& primitive,
    ImportedSkeletonIndex skeletonIndex,
    std::string_view primitiveLabel,
    const ImportedScene& scene)
{
	if (geometry.deformation.morphTargets.size() != primitive.targets_count)
	{
		throw Diagnostics::Error(std::format("glTF {} has incomplete morph-target data.", primitiveLabel));
	}
	if (geometry.HasMorphTargets() && !geometry.HasSkinInfluences())
	{
		throw Diagnostics::Error(
		    std::format("glTF {} has morph targets but no skin; morph-only deformation is unsupported.", primitiveLabel));
	}
	if (geometry.HasSkinInfluences())
	{
		if (skeletonIndex == kInvalidImportedSkeletonIndex || skeletonIndex >= scene.skeletons.size())
			throw Diagnostics::Error(std::format("glTF {} has skin influences without a bound skeleton.", primitiveLabel));
		ValidateSkinInfluences(geometry, scene.skeletons[skeletonIndex], primitiveLabel);
	}
}

std::vector<float> GltfGeometryImporter::BuildMorphWeights(
    const NodeImportContext& context,
    const ImportedMeshGeometry& geometry,
    std::string_view primitiveLabel)
{
	if (!geometry.HasSkinInfluences() || !geometry.HasMorphTargets())
	{
		return {};
	}

	std::vector<float> morphWeights = GltfMorphTargetImporter::BuildNodeMorphWeights(
	    *context.Node.mesh,
	    context.Node.weights,
	    context.Node.weights_count,
	    geometry.deformation.morphTargets.size());
	if (morphWeights.size() != geometry.deformation.morphTargets.size())
	{
		throw Diagnostics::Error(std::format("glTF {} has an invalid morph-weight assignment.", primitiveLabel));
	}
	return morphWeights;
}

void GltfGeometryImporter::AppendInstances(
    const NodeImportContext& context,
    ImportedMeshPrimitiveIndex primitiveIndex,
    ImportedMaterialIndex materialIndex,
    std::span<const float> morphWeights,
    SourceImportOutput& output)
{
	const std::string_view nodeName = context.Node.name != nullptr ? std::string_view(context.Node.name) : std::string_view();
	if (context.MeshGpuInstancingTransforms == nullptr)
	{
		GltfMeshInstanceAppender::AppendMeshInstance(
		    output,
		    primitiveIndex,
		    materialIndex,
		    context.WorldTransform,
		    kInvalidImportedMeshInstanceGroupIndex,
		    context.SkeletonIndex,
		    context.NodeIndex,
		    nodeName,
		    morphWeights);
		return;
	}

	GltfMeshInstanceAppender::AppendMeshGpuInstancingGroup(
	    output,
	    *context.MeshGpuInstancingTransforms,
	    primitiveIndex,
	    materialIndex,
	    context.WorldTransform,
	    context.SkeletonIndex,
	    context.NodeIndex,
	    nodeName,
	    morphWeights);
}

void GltfGeometryImporter::ValidateSkinInfluences(
    const ImportedMeshGeometry& geometry,
    const ImportedSkeleton& skeleton,
    std::string_view primitiveLabel)
{
	for (const ImportedSkinInfluence& influence : geometry.deformation.skinInfluences)
	{
		for (std::size_t influenceIndex = 0; influenceIndex < 8u; ++influenceIndex)
		{
			if (influence.jointWeights[influenceIndex] > 0.0f && influence.jointIndices[influenceIndex] >= skeleton.joints.size())
				throw Diagnostics::Error(std::format("glTF {} has skin influences outside its bound skeleton.", primitiveLabel));
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
