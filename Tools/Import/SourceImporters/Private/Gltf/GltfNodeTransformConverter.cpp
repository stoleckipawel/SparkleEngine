#include "PCH.h"

#include "Gltf/GltfNodeTransformConverter.h"

#include <cgltf.h>

#include <cstring>
#include <utility>

DirectX::XMMATRIX GltfNodeTransformConverter::ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept
{
	const DirectX::XMMATRIX handedness = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
	return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(handedness, matrix), handedness);
}

DirectX::XMFLOAT3 GltfNodeTransformConverter::ConvertGltfVectorToEngine(const DirectX::XMFLOAT3& value) noexcept
{
	return DirectX::XMFLOAT3(value.x, value.y, -value.z);
}

DirectX::XMFLOAT4 GltfNodeTransformConverter::ConvertGltfTangentToEngine(const DirectX::XMFLOAT4& value) noexcept
{
	return DirectX::XMFLOAT4(value.x, value.y, -value.z, -value.w);
}

void GltfNodeTransformConverter::ConvertGltfTriangleWindingToEngine(std::vector<std::uint32_t>& indices) noexcept
{
	for (std::size_t index = 0; index + 2 < indices.size(); index += 3)
	{
		std::swap(indices[index + 1], indices[index + 2]);
	}
}

DirectX::XMMATRIX GltfNodeTransformConverter::ComputeNodeWorldTransform(const cgltf_node* node)
{
	DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();

	std::vector<const cgltf_node*> nodeChain;
	for (const cgltf_node* currentNode = node; currentNode != nullptr; currentNode = currentNode->parent)
	{
		nodeChain.push_back(currentNode);
	}

	for (auto currentNode = nodeChain.rbegin(); currentNode != nodeChain.rend(); ++currentNode)
	{
		float localMatrix[16];
		cgltf_node_transform_local(*currentNode, localMatrix);
		DirectX::XMFLOAT4X4 localTransformData;
		static_assert(sizeof(localTransformData) == sizeof(localMatrix));
		std::memcpy(&localTransformData, localMatrix, sizeof(localMatrix));
		const DirectX::XMMATRIX localTransform = DirectX::XMLoadFloat4x4(&localTransformData);
		worldTransform = DirectX::XMMatrixMultiply(worldTransform, localTransform);
	}

	return ConvertGltfMatrixToEngine(worldTransform);
}

DirectX::XMFLOAT3 GltfNodeTransformConverter::TransformDirection(
    DirectX::FXMMATRIX transform,
    const DirectX::XMFLOAT3& localDirection) noexcept
{
	const DirectX::XMVECTOR direction =
	    DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localDirection), transform));
	DirectX::XMFLOAT3 result{};
	DirectX::XMStoreFloat3(&result, direction);
	return result;
}
