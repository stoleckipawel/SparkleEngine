#pragma once

#include <DirectXMath.h>
#include <d3d12.h>

class DepthConvention
{
  public:
	static float GetClearDepth() noexcept;

	static D3D12_COMPARISON_FUNC GetDepthComparisonLessEqualFunc() noexcept;

	static D3D12_COMPARISON_FUNC GetDepthComparisonFuncEqual() noexcept;

	static DirectX::XMMATRIX CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept;

	static float LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept;

	static float DepthToNDC(float linearZ, float nearZ, float farZ) noexcept;

  private:
	static constexpr bool IsReversedZ() noexcept { return true; }

	DepthConvention() = delete;
	~DepthConvention() = delete;
};
