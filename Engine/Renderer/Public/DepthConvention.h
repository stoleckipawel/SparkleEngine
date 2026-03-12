#pragma once

#include "Event.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <cstdint>

enum class DepthMode : std::uint8_t
{
	Standard,
	ReversedZ,
	Count
};

constexpr const char* DepthModeToString(DepthMode mode) noexcept
{
	switch (mode)
	{
		case DepthMode::Standard:
			return "Standard (Near=0, Far=1)";
		case DepthMode::ReversedZ:
			return "Reversed-Z (Near=1, Far=0)";
		default:
			return "Unknown";
	}
}

class DepthConvention
{
  public:
	using OnModeChangedEvent = Event<void(DepthMode)>;

	static OnModeChangedEvent OnModeChanged;

	static void SetMode(DepthMode mode) noexcept;
	static DepthMode GetMode() noexcept;
	static bool IsReversedZ() noexcept;

	static float GetClearDepth() noexcept;

	static D3D12_COMPARISON_FUNC GetDepthComparisonLessEqualFunc() noexcept;

	static D3D12_COMPARISON_FUNC GetDepthComparisonFuncEqual() noexcept;

	static DirectX::XMMATRIX CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept;

	static float LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept;

	static float DepthToNDC(float linearZ, float nearZ, float farZ) noexcept;

  private:
	static DepthMode s_mode;

	DepthConvention() = delete;
	~DepthConvention() = delete;
};
