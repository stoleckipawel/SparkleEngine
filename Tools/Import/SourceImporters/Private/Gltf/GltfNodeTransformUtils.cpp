#include "PCH.h"

#include "Gltf/GltfNodeTransformUtils.h"

#include <cgltf.h>

#include <utility>

DirectX::XMMATRIX GltfNodeTransformUtils::ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept
{
	const DirectX::XMMATRIX handedness = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
	return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(handedness, matrix), handedness);
}

DirectX::XMFLOAT3 GltfNodeTransformUtils::ConvertGltfVectorToEngine(const DirectX::XMFLOAT3& value) noexcept
{
	return DirectX::XMFLOAT3(value.x, value.y, -value.z);
}

DirectX::XMFLOAT4 GltfNodeTransformUtils::ConvertGltfTangentToEngine(const DirectX::XMFLOAT4& value) noexcept
{
	return DirectX::XMFLOAT4(value.x, value.y, -value.z, -value.w);
}

void GltfNodeTransformUtils::ConvertGltfTriangleWindingToEngine(std::vector<std::uint32_t>& indices) noexcept
{
	for (std::size_t index = 0; index + 2 < indices.size(); index += 3)
	{
		std::swap(indices[index + 1], indices[index + 2]);
	}
}

DirectX::XMMATRIX GltfNodeTransformUtils::ComputeNodeWorldTransform(const cgltf_node* node)
{
	DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();

	const cgltf_node* nodeChain[64];
	int depth = 0;
	for (const cgltf_node* currentNode = node; currentNode != nullptr && depth < 64; currentNode = currentNode->parent)
	{
		nodeChain[depth++] = currentNode;
	}

	for (int chainIndex = depth - 1; chainIndex >= 0; --chainIndex)
	{
		float localMatrix[16];
		cgltf_node_transform_local(nodeChain[chainIndex], localMatrix);
		const DirectX::XMMATRIX localTransform =
		    DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(localMatrix));
		worldTransform = DirectX::XMMatrixMultiply(worldTransform, localTransform);
	}

	return ConvertGltfMatrixToEngine(worldTransform);
}

DirectX::XMFLOAT3 GltfNodeTransformUtils::TransformDirection(
    DirectX::FXMMATRIX transform,
    const DirectX::XMFLOAT3& localDirection) noexcept
{
	const DirectX::XMVECTOR direction =
	    DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localDirection), transform));
	DirectX::XMFLOAT3 result{};
	DirectX::XMStoreFloat3(&result, direction);
	return result;
}
