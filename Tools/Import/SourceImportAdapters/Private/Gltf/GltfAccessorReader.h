#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <vector>

struct cgltf_accessor;
struct cgltf_primitive;

class GltfAccessorReader final
{
  public:
	static const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, int type);
	static void ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices);
	static DirectX::XMFLOAT2 ReadFloat2(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMFLOAT3 ReadFloat3(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMFLOAT4 ReadFloat4(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMMATRIX ReadFloat4x4(const cgltf_accessor* accessor, std::size_t index);
};
