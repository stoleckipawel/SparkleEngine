#include "PCH.h"
#include "Frustum.h"

using namespace DirectX;

void Frustum::ExtractFromViewProjection(const XMFLOAT4X4& viewProj) noexcept
{
	const XMFLOAT4X4& m = viewProj;

	planes[Left] = XMFLOAT4(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);

	planes[Right] = XMFLOAT4(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);

	planes[Bottom] = XMFLOAT4(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);

	planes[Top] = XMFLOAT4(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);

	planes[Near] = XMFLOAT4(m._13, m._23, m._33, m._43);

	planes[Far] = XMFLOAT4(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

	for (int i = 0; i < Count; ++i)
	{
		XMVECTOR plane = XMLoadFloat4(&planes[i]);
		XMVECTOR normal = XMVectorSet(planes[i].x, planes[i].y, planes[i].z, 0.0f);
		float length = XMVectorGetX(XMVector3Length(normal));
		if (length > 0.0001f)
		{
			plane = XMVectorScale(plane, 1.0f / length);
			XMStoreFloat4(&planes[i], plane);
		}
	}
}

bool Frustum::ContainsPoint(const XMFLOAT3& point) const noexcept
{
	XMVECTOR p = XMLoadFloat3(&point);

	for (int i = 0; i < Count; ++i)
	{
		XMVECTOR plane = XMLoadFloat4(&planes[i]);

		float distance = XMVectorGetX(XMVector3Dot(plane, p)) + planes[i].w;
		if (distance < 0.0f)
		{
			return false;
		}
	}
	return true;
}

bool Frustum::IntersectsSphere(const XMFLOAT3& center, float radius) const noexcept
{
	XMVECTOR c = XMLoadFloat3(&center);

	for (int i = 0; i < Count; ++i)
	{
		XMVECTOR plane = XMLoadFloat4(&planes[i]);
		float distance = XMVectorGetX(XMVector3Dot(plane, c)) + planes[i].w;
		if (distance < -radius)
		{
			return false;
		}
	}
	return true;
}

bool Frustum::IntersectsAABB(const XMFLOAT3& min, const XMFLOAT3& max) const noexcept
{
	for (int i = 0; i < Count; ++i)
	{
		const XMFLOAT4& plane = planes[i];

		XMFLOAT3 positive;
		positive.x = (plane.x >= 0.0f) ? max.x : min.x;
		positive.y = (plane.y >= 0.0f) ? max.y : min.y;
		positive.z = (plane.z >= 0.0f) ? max.z : min.z;

		float distance = plane.x * positive.x + plane.y * positive.y + plane.z * positive.z + plane.w;
		if (distance < 0.0f)
		{
			return false;
		}
	}
	return true;
}
