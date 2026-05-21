#include "PCH.h"

#include "Diagnostics/GltfGeometryInstancingDiagnostics.h"

#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <cgltf.h>

#include <algorithm>
#include <cstdint>
#include <vector>

SourceGeometryInstancingDiagnostics GltfGeometryInstancingDiagnostics::CaptureBaseline(const cgltf_data* data)
{
	SourceGeometryInstancingDiagnostics diagnostics;
	diagnostics.uniqueMeshPrimitiveCandidateCount = CountUniqueMeshPrimitiveCandidates(data);
	diagnostics.authoredInstanceGroupCount = CountAuthoredInstanceGroups(data);
	return diagnostics;
}

void GltfGeometryInstancingDiagnostics::RecordImportedPlacements(SourceImportResult& result) noexcept
{
	SourceImportDiagnosticsRecorder::RecordGeometryInstancingPlacements(result);
}

std::size_t GltfGeometryInstancingDiagnostics::CountUniqueMeshPrimitiveCandidates(const cgltf_data* data)
{
	if (data == nullptr)
	{
		return 0;
	}

	std::vector<std::uint64_t> primitiveKeys;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (!node.mesh)
		{
			continue;
		}

		const cgltf_size meshIndex = cgltf_mesh_index(data, node.mesh);
		for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
			if (primitive.type != cgltf_primitive_type_triangles || primitive.has_draco_mesh_compression)
			{
				continue;
			}

			const std::uint64_t key = (static_cast<std::uint64_t>(meshIndex) << 32u) | static_cast<std::uint64_t>(primitiveIndex);
			if (std::find(primitiveKeys.begin(), primitiveKeys.end(), key) == primitiveKeys.end())
			{
				primitiveKeys.push_back(key);
			}
		}
	}

	return primitiveKeys.size();
}

std::size_t GltfGeometryInstancingDiagnostics::CountAuthoredInstanceGroups(const cgltf_data* data) noexcept
{
	if (data == nullptr)
	{
		return 0;
	}

	std::size_t authoredGroupCount = 0;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.mesh && node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0)
		{
			for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
			{
				const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
				if (primitive.type == cgltf_primitive_type_triangles && !primitive.has_draco_mesh_compression)
				{
					++authoredGroupCount;
				}
			}
		}
	}

	return authoredGroupCount;
}