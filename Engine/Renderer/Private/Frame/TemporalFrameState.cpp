#include "PCH.h"
#include "Frame/TemporalFrameState.h"

#include <cmath>

namespace
{
	bool HasNonZeroJitter(const DirectX::XMFLOAT2& jitter) noexcept
	{
		constexpr float kEpsilon = 1.0e-7f;
		return std::abs(jitter.x) > kEpsilon || std::abs(jitter.y) > kEpsilon;
	}
}

RenderTemporalFrameState BuildRenderTemporalFrameState(const PerTemporalConstantBufferData& temporalData) noexcept
{
	return RenderTemporalFrameState{
	    .HasJitter = HasNonZeroJitter(temporalData.JitterCurrent),
	    .HasPreviousJitter = HasNonZeroJitter(temporalData.JitterPrevious),
	    .HistoryValid = temporalData.HistoryValid != 0u,
	    .JitterCurrent = temporalData.JitterCurrent,
	    .JitterPrevious = temporalData.JitterPrevious};
}
