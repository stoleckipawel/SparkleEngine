#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <vector>

struct cgltf_node;

class GltfCoordinateConverter final
{
public:
	static DirectX::XMMATRIX ComputeNodeLocalTransform(const cgltf_node* node);
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static DirectX::XMMATRIX ConvertMatrix(DirectX::FXMMATRIX sourceRowMatrix) noexcept;
	static DirectX::XMFLOAT3 ConvertPosition(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertDirection(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertNormal(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertTranslation(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertTranslationTangent(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertMorphPositionDelta(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertMorphNormalDelta(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT3 ConvertMorphTangentDelta(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT4 ConvertTangentFrame(const DirectX::XMFLOAT4& value) noexcept;
	static DirectX::XMFLOAT4 ConvertRotation(const DirectX::XMFLOAT4& value) noexcept;
	static DirectX::XMFLOAT4 ConvertRotationTangent(const DirectX::XMFLOAT4& value) noexcept;
	static DirectX::XMMATRIX ConvertCameraOrLightWorldTransform(DirectX::FXMMATRIX nodeWorldTransform) noexcept;
	static void ConvertTriangleWinding(std::vector<std::uint32_t>& indices) noexcept;
	static DirectX::XMFLOAT3 TransformDirection(DirectX::FXMMATRIX transform, const DirectX::XMFLOAT3& localDirection) noexcept;
};
