#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

struct MeshMorphTargetDelta
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 normal = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 tangent = {0.0f, 0.0f, 0.0f};
};

struct MeshMorphTarget
{
	std::string name;
	float defaultWeight = 0.0f;
	std::vector<MeshMorphTargetDelta> deltas;

	bool IsValidForVertexCount(std::uint32_t vertexCount) const noexcept { return deltas.size() == vertexCount; }
};

struct MeshMorphData
{
	std::vector<MeshMorphTarget> targets;

	bool HasTargets() const noexcept { return !targets.empty(); }
	std::uint32_t GetTargetCount() const noexcept { return static_cast<std::uint32_t>(targets.size()); }
};

static_assert(std::is_trivially_copyable_v<MeshMorphTargetDelta>, "MeshMorphTargetDelta must be trivially copyable");
