#include "PCH.h"

#include "Frame/Builders/TemporalDataBuilder.h"

#include "Camera/RenderCamera.h"
#include "Math/MathUtils.h"
#include "Temporal/TemporalJitterPatterns.h"

#include <cmath>

class TemporalDataPolicy final
{
  public:
		static constexpr uint32_t kMaxCachedJitteredFrames = 16u;

		// Donut-style temporal AA pattern baseline: Halton(2,3) sample sequence.
		// This keeps deterministic sub-pixel offsets and avoids RNG-sensitive temporal aliasing.
		static constexpr float kCutPositionDeltaMeters = 5.0f;
		static constexpr float kCutViewDirectionDotThreshold = 0.8660254f; // ~30 degrees.
		static constexpr float kCutFovDeltaDegrees = 6.0f;
};

void TemporalDataBuilder::ResetHistory(std::string_view reason) noexcept
{
	static_cast<void>(reason);
	m_resetRequested = true;
	m_hasPreviousPose = false;
}

TemporalDataBuilder::CameraPose TemporalDataBuilder::CapturePose(
    const RenderCamera& renderCamera,
    const PerViewCameraConstantBufferData& cameraData) noexcept
{
	TemporalDataBuilder::CameraPose pose{};
	const DirectX::XMMATRIX view = renderCamera.GetViewMatrix();
	const DirectX::XMMATRIX projection = renderCamera.GetProjectionMatrix();
	const DirectX::XMMATRIX viewProj = renderCamera.GetViewProjectionMatrix();
	DirectX::XMStoreFloat4x4(&pose.ViewMTX, view);
	DirectX::XMStoreFloat4x4(&pose.ProjectionMTX, projection);
	DirectX::XMStoreFloat4x4(&pose.ViewProjMTX, viewProj);
	pose.Position = cameraData.Position;
	pose.Direction = MathUtils::Normalize3(cameraData.Direction, {0.0f, 0.0f, 1.0f});
	pose.FovYDegrees = renderCamera.GetFovYDegrees();
	return pose;
}

bool TemporalDataBuilder::IsLikelyCameraCut(const TemporalDataBuilder::CameraPose& previousPose, const CameraPose& currentPose) noexcept
{
	const DirectX::XMVECTOR previousEye = DirectX::XMLoadFloat3(&previousPose.Position);
	const DirectX::XMVECTOR currentEye = DirectX::XMLoadFloat3(&currentPose.Position);
	const DirectX::XMVECTOR eyeDelta = DirectX::XMVectorSubtract(currentEye, previousEye);
	const float eyeDeltaLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(eyeDelta));
	if (eyeDeltaLength > TemporalDataPolicy::kCutPositionDeltaMeters)
	{
		return true;
	}

	const DirectX::XMVECTOR previousDirection = DirectX::XMLoadFloat3(&previousPose.Direction);
	const DirectX::XMVECTOR currentDirection = DirectX::XMLoadFloat3(&currentPose.Direction);
	const float directionDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(previousDirection, currentDirection));
	if (directionDot < TemporalDataPolicy::kCutViewDirectionDotThreshold)
	{
		return true;
	}

	const float fovDelta = std::abs(previousPose.FovYDegrees - currentPose.FovYDegrees);
	if (fovDelta > TemporalDataPolicy::kCutFovDeltaDegrees)
	{
		return true;
	}

	return false;
}

PerTemporalConstantBufferData TemporalDataBuilder::BuildTemporalData(
	const RenderCamera& renderCamera,
	const PerViewCameraConstantBufferData& cameraData,
	const RhiViewport& viewport,
	std::uint64_t frameId) noexcept
{
	PerTemporalConstantBufferData temporalData{};
	const CameraPose currentPose = CapturePose(renderCamera, cameraData);
	const DirectX::XMFLOAT2 jitterCurrent = TemporalJitterPatterns::GenerateJitterOffset(
	    viewport.Width,
	    viewport.Height,
	    static_cast<std::uint32_t>(frameId % TemporalDataPolicy::kMaxCachedJitteredFrames),
	    TemporalJitterPatterns::Pattern::Halton);
	temporalData.JitterCurrent = jitterCurrent;
	temporalData.JitterPrevious = m_previousJitter;

	const bool hasCut = m_hasPreviousPose && IsLikelyCameraCut(m_previousPose, currentPose);
	const bool hasHistory = m_hasPreviousPose && !m_resetRequested && !hasCut;
	if (hasHistory)
	{
		temporalData.PrevViewMTX = m_previousPose.ViewMTX;
		temporalData.PrevProjectionMTX = m_previousPose.ProjectionMTX;
		temporalData.PrevViewProjMTX = m_previousPose.ViewProjMTX;
		temporalData.HistoryValid = 1u;
	}
	else
	{
		temporalData.HistoryValid = 0u;
	}

	if (m_resetRequested)
	{
		m_resetRequested = false;
	}

	m_previousPose = currentPose;
	m_previousJitter = temporalData.JitterCurrent;
	m_hasPreviousPose = true;
	return temporalData;
}
