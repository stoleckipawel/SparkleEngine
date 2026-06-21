#include "PCH.h"

#include "Frame/Builders/PerFrameDataBuilder.h"

#include "Time/Timer.h"

PerFrameConstantBufferData PerFrameDataBuilder::Build(
    const Timer& timer,
    RenderViewMode viewMode,
    RenderViewportExtent sceneExtent) const noexcept
{
	const float width = static_cast<float>(sceneExtent.Width != 0u ? sceneExtent.Width : 1u);
	const float height = static_cast<float>(sceneExtent.Height != 0u ? sceneExtent.Height : 1u);

	PerFrameConstantBufferData data{};
	data.FrameIndex = timer.GetFrameCount();
	data.TotalTime = static_cast<float>(timer.GetTotalTime(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.DeltaTime = static_cast<float>(timer.GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.ScaledTotalTime = static_cast<float>(timer.GetTotalTime(TimeDomain::Scaled, TimeUnit::Seconds));
	data.ScaledDeltaTime = static_cast<float>(timer.GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
	data.ViewModeIndex = static_cast<std::uint32_t>(viewMode);
	data.ViewportSize = DirectX::XMFLOAT2(width, height);
	data.ViewportSizeInv = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
	return data;
}
