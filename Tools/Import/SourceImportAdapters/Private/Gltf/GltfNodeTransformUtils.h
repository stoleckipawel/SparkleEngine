#pragma once

#include <DirectXMath.h>

struct cgltf_node;

class GltfNodeTransformUtils final
{
  public:
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static DirectX::XMMATRIX ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept;
	static DirectX::XMFLOAT3 TransformDirection(DirectX::FXMMATRIX transform, const DirectX::XMFLOAT3& localDirection) noexcept;
};
