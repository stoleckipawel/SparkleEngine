#pragma once

#include <DirectXMath.h>

#include <assimp/matrix4x4.h>
#include <assimp/scene.h>

#include <cstdint>

class FbxNodeTransformConverter final
{
public:
	static const aiNode* FindNode(const aiScene& scene, const aiString& name) noexcept;
	static std::uint32_t FindNodeIndex(const aiScene& scene, const aiNode& target) noexcept;
	static aiMatrix4x4 ComputeNodeWorldTransform(const aiNode& node) noexcept;
	static DirectX::XMMATRIX ConvertAssimpMatrixToEngine(const aiMatrix4x4& matrix) noexcept;
	static DirectX::XMFLOAT4X4 ConvertAssimpTransformToEngine(const aiMatrix4x4& matrix) noexcept;
	static DirectX::XMFLOAT4X4 BuildNodeAttachedTranslation(const aiNode& node, const aiVector3D& position) noexcept;
	static DirectX::XMFLOAT4X4 BuildNodeAttachedOrientation(
	    const aiNode& node,
	    const aiVector3D& position,
	    const aiVector3D& direction,
	    const aiVector3D& up);

private:
	static const aiNode* FindNode(const aiNode& node, const aiString& name) noexcept;
	static bool FindNodeIndex(const aiNode& node, const aiNode& target, std::uint32_t& nextIndex, std::uint32_t& outIndex) noexcept;
};
