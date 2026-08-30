#include "PCH.h"

#include "Fbx/FbxNodeTransformConverter.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>
#include <limits>
#include <vector>

const aiNode* FbxNodeTransformConverter::FindNode(const aiScene& scene, const aiString& name) noexcept
{
	return scene.mRootNode != nullptr ? FindNode(*scene.mRootNode, name) : nullptr;
}

std::uint32_t FbxNodeTransformConverter::FindNodeIndex(const aiScene& scene, const aiNode& target) noexcept
{
	if (scene.mRootNode == nullptr)
	{
		return (std::numeric_limits<std::uint32_t>::max)();
	}

	std::uint32_t nextIndex = 0;
	std::uint32_t index = 0;
	return FindNodeIndex(*scene.mRootNode, target, nextIndex, index) ? index : (std::numeric_limits<std::uint32_t>::max)();
}

aiMatrix4x4 FbxNodeTransformConverter::ComputeNodeWorldTransform(const aiNode& node) noexcept
{
	std::vector<const aiNode*> hierarchy;
	for (const aiNode* current = &node; current != nullptr; current = current->mParent)
	{
		hierarchy.push_back(current);
	}

	aiMatrix4x4 worldTransform;
	for (auto current = hierarchy.rbegin(); current != hierarchy.rend(); ++current)
	{
		worldTransform *= (*current)->mTransformation;
	}
	return worldTransform;
}

DirectX::XMMATRIX FbxNodeTransformConverter::ConvertAssimpMatrixToEngine(const aiMatrix4x4& matrix) noexcept
{
	return DirectX::XMMATRIX(
	    matrix.a1,
	    matrix.b1,
	    matrix.c1,
	    matrix.d1,
	    matrix.a2,
	    matrix.b2,
	    matrix.c2,
	    matrix.d2,
	    matrix.a3,
	    matrix.b3,
	    matrix.c3,
	    matrix.d3,
	    matrix.a4,
	    matrix.b4,
	    matrix.c4,
	    matrix.d4);
}

DirectX::XMFLOAT4X4 FbxNodeTransformConverter::ConvertAssimpTransformToEngine(const aiMatrix4x4& matrix) noexcept
{
	DirectX::XMFLOAT4X4 transform;
	DirectX::XMStoreFloat4x4(&transform, ConvertAssimpMatrixToEngine(matrix));
	return transform;
}

DirectX::XMFLOAT4X4 FbxNodeTransformConverter::BuildNodeAttachedTranslation(const aiNode& node, const aiVector3D& position) noexcept
{
	const DirectX::XMMATRIX nodeWorld = ConvertAssimpMatrixToEngine(ComputeNodeWorldTransform(node));
	const DirectX::XMMATRIX worldTransform = DirectX::XMMatrixTranslation(position.x, position.y, position.z) * nodeWorld;

	DirectX::XMFLOAT4X4 result;
	DirectX::XMStoreFloat4x4(&result, worldTransform);
	return result;
}

DirectX::XMFLOAT4X4 FbxNodeTransformConverter::BuildNodeAttachedOrientation(
    const aiNode& node,
    const aiVector3D& position,
    const aiVector3D& direction,
    const aiVector3D& up)
{
	const DirectX::XMVECTOR localPosition = DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f);
	const DirectX::XMVECTOR localDirection = DirectX::XMVectorSet(direction.x, direction.y, direction.z, 0.0f);
	const DirectX::XMVECTOR localUp = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(localDirection)) <= 1.0e-8f
	    || DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(localUp)) <= 1.0e-8f
	    || DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMVector3Cross(localUp, localDirection))) <= 1.0e-8f)
	{
		throw Diagnostics::Error("FBX node attachment has no usable orientation basis.");
	}

	DirectX::XMVECTOR determinant;
	const DirectX::XMMATRIX localWorld =
	    DirectX::XMMatrixInverse(&determinant, DirectX::XMMatrixLookToLH(localPosition, localDirection, localUp));
	if (std::abs(DirectX::XMVectorGetX(determinant)) <= 1.0e-8f)
	{
		throw Diagnostics::Error("FBX node attachment orientation is singular.");
	}

	const DirectX::XMMATRIX worldTransform = localWorld * ConvertAssimpMatrixToEngine(ComputeNodeWorldTransform(node));
	DirectX::XMFLOAT4X4 result;
	DirectX::XMStoreFloat4x4(&result, worldTransform);
	return result;
}

const aiNode* FbxNodeTransformConverter::FindNode(const aiNode& node, const aiString& name) noexcept
{
	if (node.mName == name)
	{
		return &node;
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		if (const aiNode* match = FindNode(*node.mChildren[childIndex], name))
		{
			return match;
		}
	}
	return nullptr;
}

bool FbxNodeTransformConverter::FindNodeIndex(
    const aiNode& node,
    const aiNode& target,
    std::uint32_t& nextIndex,
    std::uint32_t& outIndex) noexcept
{
	const std::uint32_t nodeIndex = nextIndex++;
	if (&node == &target)
	{
		outIndex = nodeIndex;
		return true;
	}

	for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
	{
		if (FindNodeIndex(*node.mChildren[childIndex], target, nextIndex, outIndex))
		{
			return true;
		}
	}
	return false;
}
