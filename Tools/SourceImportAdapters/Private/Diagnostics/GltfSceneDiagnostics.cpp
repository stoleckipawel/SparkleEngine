#include "PCH.h"

#include "Diagnostics/GltfSceneDiagnostics.h"

#include <cgltf.h>

SourceSceneFeatureDiagnostics GltfSceneDiagnostics::CaptureFeatures(const cgltf_data* data) noexcept
{
	SourceSceneFeatureDiagnostics diagnostics;
	if (data == nullptr)
	{
		return diagnostics;
	}

	diagnostics.animationCount = data->animations_count;
	diagnostics.materialVariantCount = data->variants_count;

	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		diagnostics.cameraNodeCount += node.camera != nullptr ? 1u : 0u;
		diagnostics.lightNodeCount += node.light != nullptr ? 1u : 0u;
		diagnostics.skinnedNodeCount += node.skin != nullptr ? 1u : 0u;
		diagnostics.weightedNodeCount += node.weights_count > 0 ? 1u : 0u;
		diagnostics.meshGpuInstancingNodeCount += node.has_mesh_gpu_instancing ? 1u : 0u;
	}

	for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
	{
		const cgltf_mesh& mesh = data->meshes[meshIndex];
		for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = mesh.primitives[primitiveIndex];
			diagnostics.morphTargetPrimitiveCount += primitive.targets_count > 0 ? 1u : 0u;
			diagnostics.materialVariantPrimitiveCount += primitive.mappings_count > 0 ? 1u : 0u;
		}
	}

	for (cgltf_size imageIndex = 0; imageIndex < data->images_count; ++imageIndex)
	{
		diagnostics.embeddedTextureCount += data->images[imageIndex].buffer_view != nullptr ? 1u : 0u;
	}

	return diagnostics;
}