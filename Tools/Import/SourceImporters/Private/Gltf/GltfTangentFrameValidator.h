#pragma once

#include "Types/ImportedGeometry.h"

#include <DirectXMath.h>

#include <cstddef>
#include <string_view>

class GltfTangentFrameValidator final
{
public:
	static void Validate(const ImportedMeshGeometry& geometry);

private:
	static constexpr float kMinimumDirectionLengthSquared = 1.0e-12f;
	static constexpr float kUnitFrameTolerance = 1.0e-3f;
	static constexpr float kParallelTolerance = 1.0e-5f;

	static bool IsFinite(const DirectX::XMFLOAT3& value) noexcept;
	static void ValidateBaseFrame(const ImportedVertex& vertex, std::size_t vertexIndex);
	static void ValidateMorphFrame(
	    const ImportedVertex& vertex,
	    const ImportedMorphTargetDelta& delta,
	    std::size_t targetIndex,
	    std::size_t vertexIndex);
	static void ValidateDirections(const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT3& tangent, std::string_view frameLabel);
};
