#include "PCH.h"
#include "Frustum.h"

enum class FrustumPlane : std::uint8_t
{
	Left = 0,
	Right,
	Bottom,
	Top,
	Near,
	Far,
	Count
};

static constexpr std::size_t FrustumPlaneIndex(FrustumPlane plane) noexcept
{
	return static_cast<std::size_t>(plane);
}

static_assert(FrustumPlaneIndex(FrustumPlane::Count) == Frustum::kPlaneCount);

void Frustum::ExtractFromViewProjection(const DirectX::XMFLOAT4X4& viewProj) noexcept
{
	const DirectX::XMFLOAT4X4& m = viewProj;

	planes[FrustumPlaneIndex(FrustumPlane::Left)] = DirectX::XMFLOAT4(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);

	planes[FrustumPlaneIndex(FrustumPlane::Right)] = DirectX::XMFLOAT4(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);

	planes[FrustumPlaneIndex(FrustumPlane::Bottom)] = DirectX::XMFLOAT4(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);

	planes[FrustumPlaneIndex(FrustumPlane::Top)] = DirectX::XMFLOAT4(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);

	planes[FrustumPlaneIndex(FrustumPlane::Near)] = DirectX::XMFLOAT4(m._13, m._23, m._33, m._43);

	planes[FrustumPlaneIndex(FrustumPlane::Far)] = DirectX::XMFLOAT4(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

	for (DirectX::XMFLOAT4& plane : planes)
	{
		DirectX::XMVECTOR planeVector = DirectX::XMLoadFloat4(&plane);
		const DirectX::XMVECTOR normal = DirectX::XMVectorSet(plane.x, plane.y, plane.z, 0.0f);
		float length = DirectX::XMVectorGetX(DirectX::XMVector3Length(normal));
		if (length > 0.0001f)
		{
			planeVector = DirectX::XMVectorScale(planeVector, 1.0f / length);
			DirectX::XMStoreFloat4(&plane, planeVector);
		}
	}
}
