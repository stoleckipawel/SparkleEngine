#include "PCH.h"

#include "Gltf/GltfAccessorReader.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <format>
#include <limits>

const cgltf_accessor* GltfAccessorReader::FindAttribute(const cgltf_primitive& primitive, int type, int index)
{
	for (cgltf_size attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		if (primitive.attributes[attributeIndex].type == static_cast<cgltf_attribute_type>(type)
		    && primitive.attributes[attributeIndex].index == index)
		{
			return primitive.attributes[attributeIndex].data;
		}
	}

	return nullptr;
}

std::vector<std::uint32_t> GltfAccessorReader::ReadIndices(const cgltf_accessor* accessor)
{
	if (accessor == nullptr || accessor->count > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error("glTF index accessor is missing or exceeds the engine index range.");
	}

	std::vector<std::uint32_t> indices(accessor->count);
	for (cgltf_size index = 0; index < accessor->count; ++index)
	{
		const cgltf_size value = cgltf_accessor_read_index(accessor, index);
		if (value > (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error(std::format("glTF index accessor element {} exceeds the engine index range.", index));
		}
		indices[index] = static_cast<std::uint32_t>(value);
	}
	return indices;
}

DirectX::XMFLOAT2 GltfAccessorReader::ReadFloat2(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT2 element{};
	if (accessor == nullptr || index >= accessor->count
	    || !cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 2))
	{
		throw Diagnostics::Error(std::format("Cannot decode glTF float2 accessor element {}.", index));
	}
	return element;
}

DirectX::XMFLOAT3 GltfAccessorReader::ReadFloat3(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT3 element{};
	if (accessor == nullptr || index >= accessor->count
	    || !cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 3))
	{
		throw Diagnostics::Error(std::format("Cannot decode glTF float3 accessor element {}.", index));
	}
	return element;
}

DirectX::XMFLOAT4 GltfAccessorReader::ReadFloat4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4 element{};
	if (accessor == nullptr || index >= accessor->count
	    || !cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 4))
	{
		throw Diagnostics::Error(std::format("Cannot decode glTF float4 accessor element {}.", index));
	}
	return element;
}

DirectX::XMMATRIX GltfAccessorReader::ReadFloat4x4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4X4 element{};
	if (accessor == nullptr || index >= accessor->count
	    || !cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 16))
	{
		throw Diagnostics::Error(std::format("Cannot decode glTF float4x4 accessor element {}.", index));
	}
	return DirectX::XMLoadFloat4x4(&element);
}
