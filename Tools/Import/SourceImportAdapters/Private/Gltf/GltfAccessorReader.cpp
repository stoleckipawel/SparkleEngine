#include "PCH.h"

#include "Gltf/GltfAccessorReader.h"

#include <cgltf.h>

const cgltf_accessor* GltfAccessorReader::FindAttribute(const cgltf_primitive& primitive, int type)
{
	for (cgltf_size attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		if (primitive.attributes[attributeIndex].type == static_cast<cgltf_attribute_type>(type))
		{
			return primitive.attributes[attributeIndex].data;
		}
	}

	return nullptr;
}

void GltfAccessorReader::ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices)
{
	if (!accessor)
	{
		return;
	}

	outIndices.resize(accessor->count);
	for (cgltf_size index = 0; index < accessor->count; ++index)
	{
		outIndices[index] = static_cast<std::uint32_t>(cgltf_accessor_read_index(accessor, index));
	}
}

DirectX::XMFLOAT2 GltfAccessorReader::ReadFloat2(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT2 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT2) / sizeof(float));
	}

	return element;
}

DirectX::XMFLOAT3 GltfAccessorReader::ReadFloat3(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT3 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT3) / sizeof(float));
	}

	return element;
}

DirectX::XMFLOAT4 GltfAccessorReader::ReadFloat4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT4) / sizeof(float));
	}

	return element;
}

DirectX::XMMATRIX GltfAccessorReader::ReadFloat4x4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4X4 element = {
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 16);
	}

	return DirectX::XMLoadFloat4x4(&element);
}
