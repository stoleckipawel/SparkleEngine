#include "PCH.h"
#include "Frame/Temporal/TemporalFrameState.h"

#include <cmath>

class TemporalFrameStateValidation final
{
  public:
	static bool HasNonZeroJitter(const DirectX::XMFLOAT2& jitter) noexcept
	{
		constexpr float kEpsilon = 1.0e-7f;
		return std::abs(jitter.x) > kEpsilon || std::abs(jitter.y) > kEpsilon;
	}
};

RenderTemporalFrameState BuildRenderTemporalFrameState(const PerTemporalConstantBufferData& temporalData) noexcept
{
	return RenderTemporalFrameState{
	    .HasJitter = TemporalFrameStateValidation::HasNonZeroJitter(temporalData.JitterCurrent),
	    .HasPreviousJitter = TemporalFrameStateValidation::HasNonZeroJitter(temporalData.JitterPrevious),
	    .HistoryValid = temporalData.HistoryValid != 0u,
	    .JitterCurrent = temporalData.JitterCurrent,
	    .JitterPrevious = temporalData.JitterPrevious};
}
