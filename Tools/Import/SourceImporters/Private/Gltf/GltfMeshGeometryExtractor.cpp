#include "PCH.h"

#include "Gltf/GltfMeshGeometryExtractor.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfMeshTangentGenerator.h"
#include "Gltf/GltfMorphTargetImporter.h"
#include "Gltf/GltfNodeTransformConverter.h"
#include "Gltf/GltfSkinImporter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstdint>
#include <limits>

class GltfGeometryRequirements final
{
  public:
	static bool RequiresTextureCoordinates(const cgltf_material* material) noexcept
	{
		return material != nullptr &&
		       ((material->has_pbr_metallic_roughness &&
		         (material->pbr_metallic_roughness.base_color_texture.texture != nullptr ||
		          material->pbr_metallic_roughness.metallic_roughness_texture.texture != nullptr)) ||
		        material->normal_texture.texture != nullptr || material->occlusion_texture.texture != nullptr ||
		        material->emissive_texture.texture != nullptr);
	}

	static bool RequiresTangents(const cgltf_material* material) noexcept
	{
		return material != nullptr && material->normal_texture.texture != nullptr;
	}

	static bool IsNonZero(const DirectX::XMFLOAT3& value) noexcept
	{
		return value.x * value.x + value.y * value.y + value.z * value.z > 1.0e-8f;
	}
};

struct GltfMeshGeometryExtractor::Attributes
{
	const cgltf_accessor* Positions = nullptr;
	const cgltf_accessor* Normals = nullptr;
	const cgltf_accessor* TextureCoordinates = nullptr;
	const cgltf_accessor* Tangents = nullptr;
	const cgltf_accessor* Colors = nullptr;
	const cgltf_accessor* Joints0 = nullptr;
	const cgltf_accessor* Weights0 = nullptr;
	const cgltf_accessor* Joints1 = nullptr;
	const cgltf_accessor* Weights1 = nullptr;

	bool HasSkinInfluences() const noexcept { return Joints0 != nullptr && Weights0 != nullptr; }
};

GltfMeshGeometryExtractor::Attributes GltfMeshGeometryExtractor::CollectAttributes(const cgltf_primitive& primitive) noexcept
{
	return Attributes{
	    .Positions = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_position),
	    .Normals = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_normal),
	    .TextureCoordinates = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_texcoord),
	    .Tangents = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_tangent),
	    .Colors = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_color),
	    .Joints0 = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_joints, 0),
	    .Weights0 = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_weights, 0),
	    .Joints1 = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_joints, 1),
	    .Weights1 = GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_weights, 1)};
}

void GltfMeshGeometryExtractor::ValidateAttributes(const cgltf_primitive& primitive, const Attributes& attributes)
{
	if (attributes.Positions == nullptr || attributes.Normals == nullptr ||
	    (GltfGeometryRequirements::RequiresTextureCoordinates(primitive.material) && attributes.TextureCoordinates == nullptr) ||
	    attributes.Positions->count == 0 || attributes.Positions->count > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error("glTF primitive contains unsupported or malformed vertex attributes.");
	}

	for (cgltf_size attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		const cgltf_attribute& attribute = primitive.attributes[attributeIndex];
		const bool supported = ((attribute.type == cgltf_attribute_type_position || attribute.type == cgltf_attribute_type_normal ||
		                         attribute.type == cgltf_attribute_type_tangent || attribute.type == cgltf_attribute_type_texcoord ||
		                         attribute.type == cgltf_attribute_type_color) &&
		                        attribute.index == 0) ||
		                       ((attribute.type == cgltf_attribute_type_joints || attribute.type == cgltf_attribute_type_weights) &&
		                        (attribute.index == 0 || attribute.index == 1));
		if (!supported || attribute.data == nullptr || attribute.data->count != attributes.Positions->count)
		{
			throw Diagnostics::Error("glTF primitive contains unsupported or malformed vertex attributes.");
		}
	}

	if (attributes.Positions->type != cgltf_type_vec3 || attributes.Normals->type != cgltf_type_vec3 ||
	    (attributes.TextureCoordinates != nullptr && attributes.TextureCoordinates->type != cgltf_type_vec2) ||
	    (attributes.Tangents != nullptr && attributes.Tangents->type != cgltf_type_vec4) ||
	    (attributes.Colors != nullptr && attributes.Colors->type != cgltf_type_vec3 && attributes.Colors->type != cgltf_type_vec4) ||
	    (attributes.Joints0 == nullptr) != (attributes.Weights0 == nullptr) ||
	    (attributes.Joints1 == nullptr) != (attributes.Weights1 == nullptr) ||
	    GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_joints, 2) != nullptr ||
	    GltfAccessorReader::FindAttribute(primitive, cgltf_attribute_type_weights, 2) != nullptr)
	{
		throw Diagnostics::Error("glTF primitive contains unsupported or malformed vertex attributes.");
	}
}

void GltfMeshGeometryExtractor::PopulateVertices(
    const Attributes& attributes,
    std::uint32_t vertexCount,
    ImportedMeshGeometry& geometry)
{
	for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		ImportedVertex& vertex = geometry.vertices[vertexIndex];
		const DirectX::XMFLOAT3 sourcePosition = GltfAccessorReader::ReadFloat3(attributes.Positions, vertexIndex);
		vertex.position = GltfNodeTransformConverter::ConvertGltfVectorToEngine(sourcePosition);

		const DirectX::XMFLOAT3 sourceNormal = GltfAccessorReader::ReadFloat3(attributes.Normals, vertexIndex);
		if (!GltfGeometryRequirements::IsNonZero(sourceNormal))
		{
			throw Diagnostics::Error("glTF primitive contains a zero-length vertex normal.");
		}
		vertex.normal = GltfNodeTransformConverter::ConvertGltfVectorToEngine(sourceNormal);

		if (attributes.TextureCoordinates != nullptr)
		{
			vertex.uv = GltfAccessorReader::ReadFloat2(attributes.TextureCoordinates, vertexIndex);
		}

		if (attributes.Tangents != nullptr)
		{
			const DirectX::XMFLOAT4 sourceTangent = GltfAccessorReader::ReadFloat4(attributes.Tangents, vertexIndex);
			if (!GltfGeometryRequirements::IsNonZero({sourceTangent.x, sourceTangent.y, sourceTangent.z}) ||
			    (sourceTangent.w != -1.0f && sourceTangent.w != 1.0f))
			{
				throw Diagnostics::Error("glTF primitive contains an unusable vertex tangent.");
			}
			vertex.tangent = GltfNodeTransformConverter::ConvertGltfTangentToEngine(sourceTangent);
		}

		if (attributes.Colors != nullptr)
		{
			if (attributes.Colors->type == cgltf_type_vec4)
			{
				vertex.color = GltfAccessorReader::ReadFloat4(attributes.Colors, vertexIndex);
			}
			else
			{
				const DirectX::XMFLOAT3 sourceColor = GltfAccessorReader::ReadFloat3(attributes.Colors, vertexIndex);
				vertex.color = {sourceColor.x, sourceColor.y, sourceColor.z, 1.0f};
			}
		}

		if (attributes.HasSkinInfluences())
		{
			geometry.deformation.skinInfluences[vertexIndex] = GltfSkinImporter::ReadSkinInfluence(
			    attributes.Joints0,
			    attributes.Weights0,
			    attributes.Joints1,
			    attributes.Weights1,
			    vertexIndex);
		}
	}
}

void GltfMeshGeometryExtractor::PopulateIndices(
    const cgltf_primitive& primitive,
    std::uint32_t vertexCount,
    ImportedMeshGeometry& geometry)
{
	geometry.indices = GltfAccessorReader::ReadIndices(primitive.indices);
	if (geometry.indices.size() < 3u || geometry.indices.size() % 3u != 0u)
	{
		throw Diagnostics::Error("glTF primitive contains incomplete triangle-index data.");
	}
	for (const std::uint32_t index : geometry.indices)
	{
		if (index >= vertexCount)
		{
			throw Diagnostics::Error("glTF primitive contains an out-of-range triangle index.");
		}
	}
	GltfNodeTransformConverter::ConvertGltfTriangleWindingToEngine(geometry.indices);
}

ImportedMeshGeometry GltfMeshGeometryExtractor::ExtractMeshGeometry(
    const cgltf_mesh& mesh,
    const cgltf_primitive& primitive)
{
	const Attributes attributes = CollectAttributes(primitive);
	ValidateAttributes(primitive, attributes);

	const std::uint32_t vertexCount = static_cast<std::uint32_t>(attributes.Positions->count);
	ImportedMeshGeometry geometry;
	geometry.Reserve(vertexCount, primitive.indices ? static_cast<std::uint32_t>(primitive.indices->count) : 0u);
	geometry.vertices.resize(vertexCount);
	if (attributes.HasSkinInfluences())
	{
		geometry.deformation.skinInfluences.resize(vertexCount);
	}

	PopulateVertices(attributes, vertexCount, geometry);
	PopulateIndices(primitive, vertexCount, geometry);
	if (GltfGeometryRequirements::RequiresTangents(primitive.material) && attributes.Tangents == nullptr)
	{
		GltfMeshTangentGenerator::GenerateTangents(geometry);
	}
	geometry.deformation.morphTargets = GltfMorphTargetImporter::ImportMorphTargets(mesh, primitive, vertexCount);
	return geometry;
}
