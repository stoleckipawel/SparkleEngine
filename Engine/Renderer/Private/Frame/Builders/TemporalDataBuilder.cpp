#include "PCH.h"

#include "Frame/Builders/TemporalDataBuilder.h"

#include "Camera/RenderCamera.h"
#include "Math/MathUtils.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Temporal/TemporalJitterPatterns.h"

#include <cmath>

static const auto g_temporalDataBuilderLogger = Logging::GetOrCreateLogger("Renderer.TemporalDataBuilder");

namespace
{
constexpr uint32_t kMaxCachedJitteredFrames = 16u;
// Donut-style temporal AA pattern baseline: Halton(2,3) sample sequence.
// This keeps deterministic sub-pixel offsets and avoids RNG-sensitive temporal aliasing.
constexpr float kCutPositionDeltaMeters = 5.0f;
constexpr float kCutViewDirectionDotThreshold = 0.8660254f; // ~30 degrees.
constexpr float kCutFovDeltaDegrees = 6.0f;
}  // namespace

void TemporalDataBuilder::ResetHistory(std::string_view reason) noexcept
{
	m_resetRequested = true;
	m_resetReason = reason.empty() ? "Requested by renderer state" : std::string(reason);
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
	if (eyeDeltaLength > kCutPositionDeltaMeters)
	{
		return true;
	}

	const DirectX::XMVECTOR previousDirection = DirectX::XMLoadFloat3(&previousPose.Direction);
	const DirectX::XMVECTOR currentDirection = DirectX::XMLoadFloat3(&currentPose.Direction);
	const float directionDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(previousDirection, currentDirection));
	if (directionDot < kCutViewDirectionDotThreshold)
	{
		return true;
	}

	const float fovDelta = std::abs(previousPose.FovYDegrees - currentPose.FovYDegrees);
	if (fovDelta > kCutFovDeltaDegrees)
	{
		return true;
	}

	return false;
}

PerTemporalConstantBufferData TemporalDataBuilder::BuildTemporalData(
    const RenderCamera& renderCamera,
    const PerViewCameraConstantBufferData& cameraData,
    const RhiViewport& viewport) noexcept
{
	PerTemporalConstantBufferData temporalData{};
	const CameraPose currentPose = CapturePose(renderCamera, cameraData);
	const DirectX::XMFLOAT2 jitterCurrent = TemporalJitterPatterns::GenerateJitterOffset(
	    viewport.Width,
	    viewport.Height,
	    m_jitterIndex,
	    TemporalJitterPatterns::Pattern::Halton);
	m_jitterIndex = (m_jitterIndex + 1u) % kMaxCachedJitteredFrames;
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
		SPDLOG_LOGGER_DEBUG(g_temporalDataBuilderLogger, "Temporal history reset requested: {}", m_resetReason);
		m_resetRequested = false;
	}
	else if (!m_hasPreviousPose)
	{
		SPDLOG_LOGGER_DEBUG(g_temporalDataBuilderLogger, "Temporal history not yet available; initializing motion vectors without history.");
	}
	else if (hasCut)
	{
		const DirectX::XMVECTOR prevEye = DirectX::XMLoadFloat3(&m_previousPose.Position);
		const DirectX::XMVECTOR currEye = DirectX::XMLoadFloat3(&currentPose.Position);
		const DirectX::XMVECTOR deltaEye = DirectX::XMVectorSubtract(currEye, prevEye);
		const float deltaPosition = DirectX::XMVectorGetX(DirectX::XMVector3Length(deltaEye));
		const DirectX::XMVECTOR prevDirection = DirectX::XMLoadFloat3(&m_previousPose.Direction);
		const DirectX::XMVECTOR currDirection = DirectX::XMLoadFloat3(&currentPose.Direction);
		const float directionDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(prevDirection, currDirection));
		const float fovDelta = std::abs(m_previousPose.FovYDegrees - currentPose.FovYDegrees);
		SPDLOG_LOGGER_INFO(
		    g_temporalDataBuilderLogger,
		    "Temporal history reset due camera cut (deltaPos={}, dirDot={}, fovDelta={}).",
		    deltaPosition,
		    directionDot,
		    fovDelta);
	}

	m_previousPose = currentPose;
	m_previousJitter = temporalData.JitterCurrent;
	m_hasPreviousPose = true;
	return temporalData;
}
