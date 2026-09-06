#include "PCH.h"
#include "Frustum.h"

using namespace DirectX;

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

void Frustum::ExtractFromViewProjection(const XMFLOAT4X4& viewProj) noexcept
{
	const XMFLOAT4X4& m = viewProj;

	planes[FrustumPlaneIndex(FrustumPlane::Left)] = XMFLOAT4(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);

	planes[FrustumPlaneIndex(FrustumPlane::Right)] = XMFLOAT4(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);

	planes[FrustumPlaneIndex(FrustumPlane::Bottom)] = XMFLOAT4(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);

	planes[FrustumPlaneIndex(FrustumPlane::Top)] = XMFLOAT4(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);

	planes[FrustumPlaneIndex(FrustumPlane::Near)] = XMFLOAT4(m._13, m._23, m._33, m._43);

	planes[FrustumPlaneIndex(FrustumPlane::Far)] = XMFLOAT4(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

	for (XMFLOAT4& plane : planes)
	{
		XMVECTOR planeVector = XMLoadFloat4(&plane);
		const XMVECTOR normal = XMVectorSet(plane.x, plane.y, plane.z, 0.0f);
		float length = XMVectorGetX(XMVector3Length(normal));
		if (length > 0.0001f)
		{
			planeVector = XMVectorScale(planeVector, 1.0f / length);
			XMStoreFloat4(&plane, planeVector);
		}
	}
}
