#include "PCH.h"
#include "DepthConvention.h"

#include <cmath>

using namespace DirectX;

DepthMode DepthConvention::s_mode = DepthMode::ReversedZ;
DepthConvention::OnModeChangedEvent DepthConvention::OnModeChanged;

void DepthConvention::SetMode(DepthMode mode) noexcept
{
	if (s_mode == mode)
		return;

	s_mode = mode;
	OnModeChanged.Broadcast(mode);
}

DepthMode DepthConvention::GetMode() noexcept
{
	return s_mode;
}

bool DepthConvention::IsReversedZ() noexcept
{
	return s_mode == DepthMode::ReversedZ;
}

float DepthConvention::GetClearDepth() noexcept
{
	return IsReversedZ() ? 0.0f : 1.0f;
}

D3D12_COMPARISON_FUNC DepthConvention::GetDepthComparisonLessEqualFunc() noexcept
{
	return IsReversedZ() ? D3D12_COMPARISON_FUNC_GREATER : D3D12_COMPARISON_FUNC_LESS;
}

D3D12_COMPARISON_FUNC DepthConvention::GetDepthComparisonFuncEqual() noexcept
{
	return IsReversedZ() ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

XMMATRIX DepthConvention::CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept
{
	float sinFov, cosFov;
	XMScalarSinCos(&sinFov, &cosFov, 0.5f * fovY);

	const float height = cosFov / sinFov;
	const float width = height / aspect;

	if (IsReversedZ())
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

float DepthConvention::LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept
{
	if (IsReversedZ())
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
	if (IsReversedZ())
	{
		if (linearZ <= 0.0f)
			return 0.0f;
		return nearZ / linearZ;
	}

	const float range = farZ - nearZ;
	return (farZ * (linearZ - nearZ)) / (linearZ * range);
}
