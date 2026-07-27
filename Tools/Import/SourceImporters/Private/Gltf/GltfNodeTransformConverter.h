#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <vector>

struct cgltf_node;

class GltfNodeTransformConverter final
{
  public:
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static DirectX::XMMATRIX ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept;
	static DirectX::XMFLOAT3 ConvertGltfVectorToEngine(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT4 ConvertGltfTangentToEngine(const DirectX::XMFLOAT4& value) noexcept;
	static void ConvertGltfTriangleWindingToEngine(std::vector<std::uint32_t>& indices) noexcept;
	static DirectX::XMFLOAT3 TransformDirection(DirectX::FXMMATRIX transform, const DirectX::XMFLOAT3& localDirection) noexcept;
};
