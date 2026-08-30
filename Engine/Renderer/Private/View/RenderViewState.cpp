#include "PCH.h"

#include "View/RenderViewState.h"

#include "Core/Public/Math/WorldCoordinateSystem.h"
#include "Math/MathUtils.h"
#include "Temporal/TemporalJitterPatterns.h"
#include "View/RayTracing/RenderRayTracingViewPlanner.h"

#include <cmath>

class RenderViewStatePolicy final
{
public:
	static constexpr std::uint32_t MaximumCachedJitteredFrames = 16u;
	static constexpr float CutPositionDeltaMeters = 5.0f;
	static constexpr float CutViewDirectionDotThreshold = 0.8660254f;
	static constexpr float CutFovDeltaDegrees = 6.0f;
};

RenderViewState::RenderViewState() :
    m_rayTracingPlanner(std::make_unique<RenderRayTracingViewPlanner>())
{
}

RenderViewState::~RenderViewState() noexcept = default;

RenderViewInvalidationReason RenderViewState::CombineInvalidationReasons(
    RenderViewInvalidationReason left,
    RenderViewInvalidationReason right) noexcept
{
	return static_cast<RenderViewInvalidationReason>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

void RenderViewState::Invalidate(RenderViewInvalidationReason reason) noexcept
{
	m_pendingInvalidationReasons = CombineInvalidationReasons(m_pendingInvalidationReasons, reason);
	m_referenceLightingHistoryInvalidationHash.reset();
	m_restirLightingHistoryInvalidationHash.reset();
	m_rayTracingPlanner->Reset();
}

RayTracingPtlasPartitionPlan RenderViewState::BuildRayTracingPlan(
    const PreparedRenderScene& preparedScene,
    const DirectX::XMFLOAT3& cameraPosition) noexcept
{
	return m_rayTracingPlanner->Build(preparedScene, cameraPosition);
}

bool RenderViewState::UpdateHistoryInvalidationHash(std::optional<std::uint64_t>& previousHash, std::uint64_t currentHash) noexcept
{
	const bool invalidated = !previousHash || *previousHash != currentHash;
	previousHash = currentHash;
	return invalidated;
}

bool RenderViewState::UpdateReferenceLightingHistory(std::uint64_t invalidationHash) noexcept
{
	return UpdateHistoryInvalidationHash(m_referenceLightingHistoryInvalidationHash, invalidationHash);
}

bool RenderViewState::UpdateRestirLightingHistory(std::uint64_t invalidationHash) noexcept
{
	return UpdateHistoryInvalidationHash(m_restirLightingHistoryInvalidationHash, invalidationHash);
}

void RenderViewState::ObserveIdentityAndGenerations(const RenderViewStateBuildInput& input) noexcept
{
	if (!m_hasIdentity || !(m_identity == input.Identity))
	{
		Invalidate(RenderViewInvalidationReason::ViewIdentity);
	}
	if (m_hasIdentity && m_sceneGeneration != input.SceneGeneration)
	{
		Invalidate(RenderViewInvalidationReason::SceneGeneration);
	}
	if (m_hasIdentity && m_shaderGeneration != input.ShaderGeneration)
	{
		Invalidate(RenderViewInvalidationReason::ShaderGeneration);
	}
	if (m_hasIdentity && m_imageProviderGeneration != input.ImageProviderGeneration)
	{
		Invalidate(RenderViewInvalidationReason::ImageProviderGeneration);
	}
	if (m_hasIdentity && m_graphTopologyGeneration != input.GraphTopologyGeneration)
	{
		Invalidate(RenderViewInvalidationReason::GraphTopology);
	}
	if (input.ViewInput.CameraCut)
	{
		Invalidate(RenderViewInvalidationReason::CameraCut);
	}
	if (input.ViewInput.CameraTeleported)
	{
		Invalidate(RenderViewInvalidationReason::CameraTeleport);
	}

	m_identity = input.Identity;
	m_sceneGeneration = input.SceneGeneration;
	m_shaderGeneration = input.ShaderGeneration;
	m_imageProviderGeneration = input.ImageProviderGeneration;
	m_graphTopologyGeneration = input.GraphTopologyGeneration;
	m_hasIdentity = true;
}

RenderViewState::CameraPose RenderViewState::CapturePose(const RenderViewStateBuildInput& input) noexcept
{
	return CameraPose{
	    .WorldToViewMatrix = input.Camera.ViewMTX,
	    .ViewToClipMatrix = input.Camera.ProjectionMTX,
	    .WorldToClipMatrix = input.Camera.ViewProjMTX,
	    .Position = input.Camera.Position,
	    .Direction = MathUtils::Normalize3(
	        input.Camera.Direction,
	        {WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ}),
	    .ProjectionKind = input.ViewInput.Camera.ProjectionKind,
	    .FovYDegrees = input.ViewInput.Camera.FovYDegrees,
	    .NearZ = input.ViewInput.Camera.NearZ,
	    .FarZ = input.ViewInput.Camera.FarZ,
	    .OrthographicHeightMeters = input.ViewInput.Camera.OrthographicHeightMeters};
}

bool RenderViewState::HasProjectionChange(const CameraPose& previousPose, const CameraPose& currentPose) noexcept
{
	constexpr float tolerance = 1.0e-6f;
	return previousPose.ProjectionKind != currentPose.ProjectionKind
	    || std::abs(previousPose.FovYDegrees - currentPose.FovYDegrees) > tolerance
	    || std::abs(previousPose.NearZ - currentPose.NearZ) > tolerance || std::abs(previousPose.FarZ - currentPose.FarZ) > tolerance
	    || std::abs(previousPose.OrthographicHeightMeters - currentPose.OrthographicHeightMeters) > tolerance;
}

bool RenderViewState::IsLikelyCameraCut(const CameraPose& previousPose, const CameraPose& currentPose) noexcept
{
	const DirectX::XMVECTOR eyeDelta =
	    DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&currentPose.Position), DirectX::XMLoadFloat3(&previousPose.Position));
	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(eyeDelta)) > RenderViewStatePolicy::CutPositionDeltaMeters)
	{
		return true;
	}

	const float directionDot = DirectX::XMVectorGetX(
	    DirectX::XMVector3Dot(DirectX::XMLoadFloat3(&previousPose.Direction), DirectX::XMLoadFloat3(&currentPose.Direction)));
	if (directionDot < RenderViewStatePolicy::CutViewDirectionDotThreshold)
	{
		return true;
	}

	return std::abs(previousPose.FovYDegrees - currentPose.FovYDegrees) > RenderViewStatePolicy::CutFovDeltaDegrees;
}

ViewTemporalUniformData RenderViewState::BuildTemporal(const RenderViewStateBuildInput& input) noexcept
{
	ObserveIdentityAndGenerations(input);
	const CameraPose currentPose = CapturePose(input);
	if (m_hasPreviousPose && HasProjectionChange(m_previousPose, currentPose))
	{
		Invalidate(RenderViewInvalidationReason::ProjectionChange);
	}
	if (m_hasPreviousPose && IsLikelyCameraCut(m_previousPose, currentPose))
	{
		Invalidate(RenderViewInvalidationReason::CameraDiscontinuity);
	}

	m_temporalSampleIndex = static_cast<std::uint32_t>(input.FrameId % RenderViewStatePolicy::MaximumCachedJitteredFrames);
	ViewTemporalUniformData temporal = {};
	temporal.CurrentJitterNdc = TemporalJitterPatterns::GenerateJitterOffset(
	    static_cast<float>(input.RenderExtent.Width),
	    static_cast<float>(input.RenderExtent.Height),
	    m_temporalSampleIndex,
	    TemporalJitterPatterns::Pattern::Halton);
	temporal.PreviousJitterNdc = m_previousJitterNdc;

	const bool historyValid = m_hasPreviousPose && m_pendingInvalidationReasons == RenderViewInvalidationReason::None;
	if (historyValid)
	{
		temporal.PreviousWorldToViewMatrix = m_previousPose.WorldToViewMatrix;
		temporal.PreviousViewToClipMatrix = m_previousPose.ViewToClipMatrix;
		temporal.PreviousWorldToClipMatrix = m_previousPose.WorldToClipMatrix;
		temporal.HistoryValid = 1u;
	}

	m_previousPose = currentPose;
	m_previousJitterNdc = temporal.CurrentJitterNdc;
	m_hasPreviousPose = true;
	m_lastInvalidationReasons = m_pendingInvalidationReasons;
	m_pendingInvalidationReasons = RenderViewInvalidationReason::None;
	return temporal;
}
