#include "PCH.h"
#include "SceneData/Input/Validation/RenderFrameMetadataValidator.h"

#include <cmath>

namespace
{
	bool IsFinitePositive(float value) noexcept
	{
		return std::isfinite(value) && value > 0.0f;
	}
}

bool RenderFrameMetadataValidator::Validate(
    const RenderInputFrame& input,
    bool& historyResetRequired,
    std::string& diagnostic) const
{
	const RenderFrameMetadata& metadata = input.Dynamic.Metadata;
	if (metadata.FrameGeneration == 0 || metadata.FrameGeneration != input.WorldDelta.SceneGeneration)
	{
		diagnostic = "Render input frame generation does not match its structural delta.";
		return false;
	}
	if (m_hasAcceptedFrame && metadata.FrameId <= m_frameId)
	{
		diagnostic = "Render input frame identity is stale or duplicated.";
		return false;
	}
	if (metadata.RenderWidth == 0 || metadata.RenderHeight == 0 || metadata.OutputWidth == 0 ||
	    metadata.OutputHeight == 0 || !IsFinitePositive(metadata.Exposure))
	{
		diagnostic = "Render input frame resolution or exposure metadata is invalid.";
		return false;
	}
	if (metadata.MotionVectors != RenderMotionVectorConvention::CurrentToPreviousPixels ||
	    metadata.Depth != RenderDepthConvention::ReversedZZeroToOne)
	{
		diagnostic = "Render input uses an unsupported motion-vector or depth convention.";
		return false;
	}
	if ((metadata.CameraCut || metadata.CameraTeleported) && !metadata.ResetHistory)
	{
		diagnostic = "Render input camera discontinuity must explicitly reset temporal history.";
		return false;
	}
	if (!IsFinitePositive(input.Dynamic.Camera.AspectRatio) || !IsFinitePositive(input.Dynamic.Camera.NearZ) ||
	    !std::isfinite(input.Dynamic.Camera.FarZ) || input.Dynamic.Camera.FarZ <= input.Dynamic.Camera.NearZ ||
	    !IsFinitePositive(input.Dynamic.Camera.FovYDegrees))
	{
		diagnostic = "Render input camera metadata is invalid.";
		return false;
	}

	historyResetRequired = metadata.ResetHistory ||
	                       (m_hasAcceptedFrame && (metadata.FrameGeneration != m_frameGeneration ||
	                                                metadata.ProviderGeneration != m_providerGeneration));
	return true;
}

void RenderFrameMetadataValidator::Commit(const RenderFrameMetadata& metadata) noexcept
{
	m_frameId = metadata.FrameId;
	m_frameGeneration = metadata.FrameGeneration;
	m_providerGeneration = metadata.ProviderGeneration;
	m_hasAcceptedFrame = true;
}
