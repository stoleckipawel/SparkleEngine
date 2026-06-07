#include "PCH.h"

#include "Gltf/GltfMeshGeometryExtractor.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformUtils.h"
#include "Gltf/GltfSkinImporter.h"

#include <cgltf.h>

#include <cstdint>

ImportedMeshGeometry GltfMeshGeometryExtractor::ExtractMeshGeometry(const cgltf_primitive& primitive)
{
	const cgltf_accessor* positions = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_position);
	const cgltf_accessor* normals = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_normal);
	const cgltf_accessor* texcoords = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_texcoord);
	const cgltf_accessor* tangents = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_tangent);
	const cgltf_accessor* joints = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_joints);
	const cgltf_accessor* weights = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_weights);

	if (!positions)
	{
		return {};
	}

	const std::uint32_t vertexCount = static_cast<std::uint32_t>(positions->count);
	ImportedMeshGeometry meshGeometry;
	meshGeometry.hasSkinInfluences = joints != nullptr && weights != nullptr;
	meshGeometry.Reserve(vertexCount, primitive.indices ? static_cast<std::uint32_t>(primitive.indices->count) : 0);
	meshGeometry.vertices.resize(vertexCount);

	for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		ImportedVertex& vertex = meshGeometry.vertices[vertexIndex];
		vertex.position = GltfNodeTransformUtils::ConvertGltfVectorToEngine(GltfAccessorReader::ReadFloat3(positions, vertexIndex));
		vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};

		if (normals)
		{
			vertex.normal = GltfNodeTransformUtils::ConvertGltfVectorToEngine(GltfAccessorReader::ReadFloat3(normals, vertexIndex));
		}

		if (texcoords)
		{
			vertex.uv = GltfAccessorReader::ReadFloat2(texcoords, vertexIndex);
		}

		if (tangents)
		{
			vertex.tangent = GltfNodeTransformUtils::ConvertGltfTangentToEngine(GltfAccessorReader::ReadFloat4(tangents, vertexIndex));
		}

		if (meshGeometry.hasSkinInfluences)
		{
			vertex.skinInfluence = GltfSkinImporter::ReadSkinInfluence(joints, weights, vertexIndex);
		}
	}

	GltfAccessorReader::ReadIndices(primitive.indices, meshGeometry.indices);
	GltfNodeTransformUtils::ConvertGltfTriangleWindingToEngine(meshGeometry.indices);
	return meshGeometry;
}
