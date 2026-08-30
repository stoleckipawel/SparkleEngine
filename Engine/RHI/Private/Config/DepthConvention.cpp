#include "PCH.h"

#include "Config/DepthConvention.h"

#include <cmath>

using namespace DirectX;

float DepthConvention::GetClearDepth() noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		return 0.0f;
	}

	return 1.0f;
}

CompareOp DepthConvention::GetDepthComparisonLessEqualFunc() noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		return CompareOp::Greater;
	}

	return CompareOp::Less;
}

CompareOp DepthConvention::GetDepthComparisonFuncEqual() noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		return CompareOp::GreaterOrEqual;
	}

	return CompareOp::LessOrEqual;
}

XMMATRIX DepthConvention::CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept
{
	float sinFov, cosFov;
	XMScalarSinCos(&sinFov, &cosFov, 0.5f * fovY);

	const float height = cosFov / sinFov;
	const float width = height / aspect;

	if (DepthConvention::IsReversedZ())
	{
		const float fRange = nearZ / (nearZ - farZ);

		XMMATRIX m;
		m.r[0] = XMVectorSet(width, 0.0f, 0.0f, 0.0f);
		m.r[1] = XMVectorSet(0.0f, height, 0.0f, 0.0f);
		m.r[2] = XMVectorSet(0.0f, 0.0f, fRange, 1.0f);
		m.r[3] = XMVectorSet(0.0f, 0.0f, -fRange * farZ, 0.0f);
		return m;
	}

	return XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
}

XMMATRIX DepthConvention::CreateOrthographicOffCenterLH(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		return XMMatrixOrthographicOffCenterLH(left, right, bottom, top, farZ, nearZ);
	}

	return XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

float DepthConvention::LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		if (ndcDepth <= 0.0f)
			return farZ;
		return nearZ / ndcDepth;
	}

	const float range = farZ - nearZ;
	return (nearZ * farZ) / (farZ - ndcDepth * range);
}

float DepthConvention::DepthToNDC(float linearZ, float nearZ, float farZ) noexcept
{
	if (DepthConvention::IsReversedZ())
	{
		(void) farZ;
		if (linearZ <= 0.0f)
			return 0.0f;
		return nearZ / linearZ;
	}

	const float range = farZ - nearZ;
	return (farZ * (linearZ - nearZ)) / (linearZ * range);
}
