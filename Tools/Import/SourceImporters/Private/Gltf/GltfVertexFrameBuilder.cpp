#include "PCH.h"

#include "Gltf/GltfVertexFrameBuilder.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>

DirectX::XMFLOAT3 GltfVertexFrameBuilder::BuildNormal(const DirectX::XMFLOAT3& normal)
{
	if (!IsFinite(normal))
	{
		throw Diagnostics::Error("glTF primitive contains a non-finite vertex normal.");
	}
	const DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&normal);
	const float lengthSquared = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(direction));
	if (lengthSquared <= kMinimumDirectionLengthSquared)
	{
		throw Diagnostics::Error("glTF primitive contains a zero-length vertex normal.");
	}
	if (std::abs(lengthSquared - 1.0f) > kUnitFrameTolerance)
	{
		throw Diagnostics::Error("glTF primitive contains a non-unit vertex normal; repair the source mesh.");
	}

	DirectX::XMFLOAT3 normalizedNormal;
	DirectX::XMStoreFloat3(&normalizedNormal, DirectX::XMVector3Normalize(direction));
	return normalizedNormal;
}

DirectX::XMFLOAT4 GltfVertexFrameBuilder::BuildAuthoredTangent(const DirectX::XMFLOAT4& tangent, const DirectX::XMFLOAT3& normalizedNormal)
{
	const DirectX::XMFLOAT3 tangentDirection{tangent.x, tangent.y, tangent.z};
	if (!IsFinite(tangentDirection) || !std::isfinite(tangent.w) || (tangent.w != -1.0f && tangent.w != 1.0f))
	{
		throw Diagnostics::Error("glTF primitive contains a non-finite tangent or invalid tangent handedness.");
	}

	const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&normalizedNormal);
	const DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&tangentDirection);
	const float lengthSquared = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(direction));
	if (lengthSquared <= kMinimumDirectionLengthSquared)
	{
		throw Diagnostics::Error("glTF primitive contains a zero-length vertex tangent.");
	}
	const DirectX::XMVECTOR projected =
	    DirectX::XMVectorSubtract(direction, DirectX::XMVectorMultiply(normal, DirectX::XMVector3Dot(normal, direction)));
	const float projectedLengthSquared = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(projected));
	if (projectedLengthSquared <= kMinimumDirectionLengthSquared)
	{
		throw Diagnostics::Error("glTF primitive contains parallel normal and tangent directions.");
	}

	DirectX::XMFLOAT3 normalizedTangent;
	DirectX::XMStoreFloat3(&normalizedTangent, DirectX::XMVector3Normalize(projected));
	return {normalizedTangent.x, normalizedTangent.y, normalizedTangent.z, tangent.w};
}

bool GltfVertexFrameBuilder::IsFinite(const DirectX::XMFLOAT3& value) noexcept
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
