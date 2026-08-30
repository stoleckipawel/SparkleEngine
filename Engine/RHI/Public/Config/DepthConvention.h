#pragma once

#include "../RHIAPI.h"
#include "../Formats/CompareOp.h"

#include <DirectXMath.h>

class SPARKLE_RHI_API DepthConvention
{
public:
	static float GetClearDepth() noexcept;

	static CompareOp GetDepthComparisonLessEqualFunc() noexcept;

	static CompareOp GetDepthComparisonFuncEqual() noexcept;

	static DirectX::XMMATRIX CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept;
	static DirectX::XMMATRIX CreateOrthographicOffCenterLH(
	    float left,
	    float right,
	    float bottom,
	    float top,
	    float nearZ,
	    float farZ) noexcept;

	static float LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept;

	static float DepthToNDC(float linearZ, float nearZ, float farZ) noexcept;

private:
	static constexpr bool IsReversedZ() noexcept { return true; }

	DepthConvention() = delete;
	~DepthConvention() = delete;
};
