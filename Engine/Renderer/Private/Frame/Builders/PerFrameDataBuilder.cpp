#include "PCH.h"

#include "Frame/Builders/PerFrameDataBuilder.h"

#include "Time/Timer.h"

PerFrameDataBuilder::PerFrameDataBuilder() noexcept = default;
PerFrameDataBuilder::~PerFrameDataBuilder() noexcept = default;

PerFrameConstantBufferData PerFrameDataBuilder::Build(
    const TimeInfo& timing,
    RenderViewMode viewMode,
    RenderViewportExtent sceneExtent) const noexcept
{
	const float width = static_cast<float>(sceneExtent.Width != 0u ? sceneExtent.Width : 1u);
	const float height = static_cast<float>(sceneExtent.Height != 0u ? sceneExtent.Height : 1u);

	PerFrameConstantBufferData data{};
	data.FrameIndex = timing.frameIndex;
	data.TotalTimeSeconds = static_cast<float>(timing.unscaledTime.count());
	data.DeltaTimeSeconds = static_cast<float>(timing.unscaledDelta.count());
	data.ScaledTotalTimeSeconds = static_cast<float>(timing.scaledTime.count());
	data.ScaledDeltaTimeSeconds = static_cast<float>(timing.scaledDelta.count());
	data.ViewModeIndex = static_cast<std::uint32_t>(viewMode);
	data.ViewportSize = DirectX::XMFLOAT2(width, height);
	data.ViewportSizeInv = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
	return data;
}
