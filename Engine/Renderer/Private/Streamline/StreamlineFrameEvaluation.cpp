#include "../PCH.h"
#include "Streamline/StreamlineFrameEvaluation.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Providers/ImageProviderFrameContext.h"
#include "Streamline/StreamlineViewConstants.h"

StreamlineFrameEvaluation::StreamlineFrameEvaluation(
    sl::ViewportHandle viewport,
    NativeGraphicsCommandListHandle commandList) noexcept :
    m_viewport(viewport), m_commandBuffer(static_cast<sl::CommandBuffer*>(commandList.Value))
{
}

bool StreamlineFrameEvaluation::AcquireFrameToken(std::uint64_t frameId) noexcept
{
	const std::uint32_t streamlineFrameIndex = static_cast<std::uint32_t>(frameId);
	return m_commandBuffer != nullptr &&
	       slGetNewFrameToken(m_frameToken, &streamlineFrameIndex) == sl::Result::eOk &&
	       m_frameToken != nullptr;
}

bool StreamlineFrameEvaluation::SetViewConstants(const ImageProviderFrameContext& frameContext) noexcept
{
	if (m_frameToken == nullptr)
	{
		return false;
	}

	sl::Constants constants{};
	FillStreamlineViewConstants(
	    constants,
	    StreamlineViewConstantsInput{
	        .Camera = frameContext.Camera,
	        .TemporalData = frameContext.TemporalData,
	        .TemporalState = frameContext.TemporalState,
	        .RenderExtent = frameContext.RenderExtent,
	        .MotionVectorsCurrentMinusPrevious = true,
	        .ReversedDeviceDepth = true,
	        .ResetRequested = frameContext.ResetHistory});
	return slSetConstants(constants, *m_frameToken, m_viewport) == sl::Result::eOk;
}

bool StreamlineFrameEvaluation::Evaluate(sl::Feature feature) noexcept
{
	if (m_frameToken == nullptr || m_commandBuffer == nullptr)
	{
		return false;
	}

	const sl::BaseStructure* inputs[] = {&m_viewport};
	return slEvaluateFeature(
	           feature,
	           *m_frameToken,
	           inputs,
	           static_cast<std::uint32_t>(std::size(inputs)),
	           m_commandBuffer) == sl::Result::eOk;
}
#endif
