#include "PCH.h"

#include "Gltf/GltfCoordinateConverter.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstring>
#include <utility>

namespace GltfCoordinateConversionDetail
{
	DirectX::XMMATRIX LoadGltfRowMatrix(const float (&values)[16]) noexcept
	{
		DirectX::XMFLOAT4X4 matrix;
		static_assert(sizeof(matrix) == sizeof(values));
		std::memcpy(&matrix, values, sizeof(values));
		return DirectX::XMLoadFloat4x4(&matrix);
	}

	DirectX::XMFLOAT3 ReflectSourceX(const DirectX::XMFLOAT3& value) noexcept
	{
		return {-value.x, value.y, value.z};
	}
}

DirectX::XMMATRIX GltfCoordinateConverter::ComputeNodeLocalTransform(const cgltf_node* node)
{
	if (node == nullptr)
	{
		throw Diagnostics::Error("Cannot convert a null glTF node transform.");
	}

	float sourceMatrix[16];
	cgltf_node_transform_local(node, sourceMatrix);
	return ConvertMatrix(GltfCoordinateConversionDetail::LoadGltfRowMatrix(sourceMatrix));
}

DirectX::XMMATRIX GltfCoordinateConverter::ComputeNodeWorldTransform(const cgltf_node* node)
{
	if (node == nullptr)
	{
		throw Diagnostics::Error("Cannot convert a null glTF node transform.");
	}

	float sourceMatrix[16];
	cgltf_node_transform_world(node, sourceMatrix);
	return ConvertMatrix(GltfCoordinateConversionDetail::LoadGltfRowMatrix(sourceMatrix));
}

DirectX::XMMATRIX GltfCoordinateConverter::ConvertMatrix(DirectX::FXMMATRIX sourceRowMatrix) noexcept
{
	const DirectX::XMMATRIX sourceToEngine = DirectX::XMMatrixScaling(-1.0f, 1.0f, 1.0f);
	return sourceToEngine * sourceRowMatrix * sourceToEngine;
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertPosition(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertDirection(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertNormal(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertTranslation(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertTranslationTangent(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertMorphPositionDelta(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertMorphNormalDelta(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT3 GltfCoordinateConverter::ConvertMorphTangentDelta(const DirectX::XMFLOAT3& value) noexcept
{
	return GltfCoordinateConversionDetail::ReflectSourceX(value);
}

DirectX::XMFLOAT4 GltfCoordinateConverter::ConvertTangentFrame(const DirectX::XMFLOAT4& value) noexcept
{
	return {-value.x, value.y, value.z, -value.w};
}

DirectX::XMFLOAT4 GltfCoordinateConverter::ConvertRotation(const DirectX::XMFLOAT4& value) noexcept
{
	return {value.x, -value.y, -value.z, value.w};
}

DirectX::XMFLOAT4 GltfCoordinateConverter::ConvertRotationTangent(const DirectX::XMFLOAT4& value) noexcept
{
	return {value.x, -value.y, -value.z, value.w};
}

DirectX::XMMATRIX GltfCoordinateConverter::ConvertCameraOrLightWorldTransform(DirectX::FXMMATRIX nodeWorldTransform) noexcept
{
	return DirectX::XMMatrixRotationY(DirectX::XM_PI) * nodeWorldTransform;
}

void GltfCoordinateConverter::ConvertTriangleWinding(std::vector<std::uint32_t>& indices) noexcept
{
	for (std::size_t index = 0; index + 2u < indices.size(); index += 3u)
	{
		std::swap(indices[index + 1u], indices[index + 2u]);
	}
}

DirectX::XMFLOAT3 GltfCoordinateConverter::TransformDirection(
    DirectX::FXMMATRIX transform,
    const DirectX::XMFLOAT3& localDirection) noexcept
{
	const DirectX::XMVECTOR direction =
	    DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localDirection), transform));
	DirectX::XMFLOAT3 result;
	DirectX::XMStoreFloat3(&result, direction);
	return result;
}
