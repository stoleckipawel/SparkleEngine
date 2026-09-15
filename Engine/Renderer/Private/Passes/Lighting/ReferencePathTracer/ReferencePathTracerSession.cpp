#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <algorithm>

ReferencePathTracerSession::ReferencePathTracerSession(RenderDeviceServices& deviceServices) noexcept :
    m_deviceServices(deviceServices)
{
}

static_assert(
    static_cast<std::uint8_t>(ReferencePathTracerIdentityComponent::Backend)
    == static_cast<std::uint8_t>(ViewportRenderProgressReason::BackendChanged));

ViewportRenderProgressReason ReferencePathTracerSession::ResolveAvailability(
    const RenderView& view,
    const PreparedRenderScene& scene) const noexcept
{
	const RhiCapabilities& capabilities = m_deviceServices.GetCapabilities();
	const RhiFormatSupport* format = capabilities.FindFormatSupport(PixelFormat::R32G32B32A32_Float);
	const MaterialTextureTableCapabilityReport materialTextures = BuildMaterialTextureTableCapabilityReport(capabilities);
	const bool supportsView = view.camera.ProjectionKind == CameraProjectionKind::Perspective && view.renderExtent.IsValid();
	if (!supportsView)
	{
		return ViewportRenderProgressReason::UnsupportedView;
	}
	const bool supportsAccumulation = format != nullptr && format->SupportsShaderResource && format->SupportsUnorderedAccess;
	const bool supportsTracing = capabilities.RayTracing.SupportsAccelerationStructure && capabilities.RayTracing.SupportsInlineRayQuery;
	if (!supportsAccumulation || !supportsTracing || !materialTextures.Supported)
	{
		return ViewportRenderProgressReason::UnsupportedCapability;
	}
	const bool unsupportedContent = std::any_of(
	    scene.materials.begin(),
	    scene.materials.end(),
	    [](const MaterialData& material) { return material.alphaMode == 2u || material.subsurfaceStrength != 0.0f; });
	return unsupportedContent ? ViewportRenderProgressReason::UnsupportedContent : ViewportRenderProgressReason::None;
}

void ReferencePathTracerSession::BeginIdentity(
    const ReferencePathTracerIdentity& identity,
    RenderViewportExtent extent,
    ViewportRenderProgressReason reason,
    std::uint32_t discardedSamples) noexcept
{
	m_identity = identity;
	m_hasIdentity = true;
	m_committedSamples = 0u;
	m_discardedSamples = discardedSamples;
	m_pixelCount = static_cast<std::uint64_t>(extent.Width) * static_cast<std::uint64_t>(extent.Height);
	m_nextRow = 0u;
	m_pendingCommit = {};
	m_workPrepared = false;
	m_clearDisplay = true;
	m_resetVisible = reason != ViewportRenderProgressReason::None;
	m_paused = m_paused || m_pausePending;
	m_pausePending = false;
	m_suspensionPending = false;
	m_samplesPerSecond = 0.0;
	m_lastCommitTime = std::chrono::steady_clock::now();
	m_lastReason = reason;
	m_unavailableReason = ViewportRenderProgressReason::None;
	++m_executionGeneration;
}

void ReferencePathTracerSession::UpdateIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    ReferencePathTracerResources& resources) noexcept
{
	const ReferencePathTracerIdentity identity =
	    BuildReferencePathTracerIdentity(view, scene, frame, sceneGeneration, m_deviceServices.GetCapabilities().BackendApi);
	if (m_hasIdentity && identity == m_identity)
	{
		return;
	}
	const ViewportRenderProgressReason reason =
	    m_hasIdentity ? static_cast<ViewportRenderProgressReason>(m_identity.FindFirstDifference(identity)) : m_lastReason;
	const std::uint32_t discardedSamples = m_hasIdentity ? m_committedSamples : m_discardedSamples;
	const ViewportRenderProgressReason availability = ResolveAvailability(view, scene);
	if (availability != ViewportRenderProgressReason::None)
	{
		m_committedSamples = 0u;
		m_discardedSamples = discardedSamples;
		m_pendingCommit = {};
		m_workPrepared = false;
		m_resetVisible = false;
		m_paused = false;
		m_pausePending = false;
		m_retentionAvailable = false;
		++m_executionGeneration;
		resources.Release();
		m_identity = identity;
		m_hasIdentity = true;
		m_hasOwner = false;
		m_lastReason = availability;
		m_unavailableReason = availability;
		return;
	}
	resources.Allocate(view.renderExtent);
	BeginIdentity(identity, view.renderExtent, reason, discardedSamples);
}

void ReferencePathTracerSession::CompletePendingCommit() noexcept
{
	if (!m_pendingCommit || !m_deviceServices.IsSubmissionComplete(m_pendingCommit.Submission))
	{
		return;
	}
	if (m_pendingCommit.ExecutionGeneration == m_executionGeneration)
	{
		m_committedSamples = m_pendingCommit.Prefix;
		const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		const double seconds = std::chrono::duration<double>(now - m_lastCommitTime).count();
		if (seconds > 0.0)
		{
			m_samplesPerSecond = static_cast<double>(m_pixelCount) / seconds;
		}
		m_lastCommitTime = now;
	}
	m_pendingCommit = {};
	if (m_pausePending)
	{
		m_pausePending = false;
		m_paused = true;
	}
}

void ReferencePathTracerSession::PrepareWork(RenderViewportExtent extent) noexcept
{
	m_uniformData = ReferencePathTracerUniformData{
	    .SessionSeed = 0u,
	    .ReplicateId = 0u,
	    .SampleOrdinal = m_committedSamples,
	    .FinitePathDiagnosticSurfaceVertices = 0u,
	    .FirstRow = m_nextRow,
	    .RowCount = (std::min) (WorkRowsPerDispatch, extent.Height - m_nextRow),
	    .PriorSampleCount = m_committedSamples,
	    .WorkFlags =
	        ReferencePathTracerUniformData::WorkFlagTrace | (m_clearDisplay ? ReferencePathTracerUniformData::WorkFlagClearDisplay : 0u)};
	m_preparedRowCount = m_uniformData.RowCount;
	if (m_nextRow + m_preparedRowCount == extent.Height)
	{
		m_uniformData.WorkFlags |= ReferencePathTracerUniformData::WorkFlagCommitPrefix;
	}
	m_workPrepared = true;
}

void ReferencePathTracerSession::ApplyAction(
    const ViewportRenderRequest& request,
    const RenderView& view,
    ReferencePathTracerResources& resources) noexcept
{
	if (request.ViewportId == m_lastActionViewportId && request.RenderActionSequence == m_lastActionSequence)
	{
		return;
	}
	m_lastActionViewportId = request.ViewportId;
	m_lastActionSequence = request.RenderActionSequence;

	switch (request.RenderAction)
	{
		case ViewportRenderAction::Pause:
			if (m_unavailableReason == ViewportRenderProgressReason::None && m_hasIdentity && resources.CanRetain())
			{
				m_workPrepared = false;
				m_pausePending = static_cast<bool>(m_pendingCommit);
				m_paused = !m_pausePending;
			}
			break;
		case ViewportRenderAction::Resume:
			if (m_paused)
			{
				m_paused = false;
				m_lastReason = ViewportRenderProgressReason::Resumed;
				m_lastCommitTime = std::chrono::steady_clock::now();
			}
			break;
		case ViewportRenderAction::Restart:
			if (m_unavailableReason == ViewportRenderProgressReason::None && m_hasIdentity)
			{
				m_paused = false;
				BeginIdentity(m_identity, view.renderExtent, ViewportRenderProgressReason::ManualRestart, m_committedSamples);
			}
			break;
		case ViewportRenderAction::Transfer:
			if (m_hasOwner && m_ownerViewportId != view.viewportId)
			{
				m_transferViewportId = view.viewportId;
				m_transferPending = true;
			}
			break;
		case ViewportRenderAction::None:
		default:
			break;
	}
}

ViewportRenderProgress ReferencePathTracerSession::Update(
    const ViewportRenderRequest& request,
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    ReferencePathTracerResources& resources) noexcept
{
	CompletePendingCommit();
	const bool active = request.ViewMode == RenderViewMode::ReferencePathTracer;
	if (!active)
	{
		m_lastActionViewportId = request.ViewportId;
		m_lastActionSequence = request.RenderActionSequence;
		m_selected = false;
		Suspend(resources);
		return {};
	}
	m_selected = true;
	ApplyAction(request, view, resources);
	if (m_transferPending && m_transferViewportId == view.viewportId && !m_pendingCommit)
	{
		m_discardedSamples = m_committedSamples;
		resources.Release();
		m_identity = {};
		m_hasIdentity = false;
		m_hasOwner = false;
		m_transferViewportId = 0u;
		m_transferPending = false;
		m_lastReason = ViewportRenderProgressReason::ViewChanged;
		m_unavailableReason = ViewportRenderProgressReason::None;
		++m_executionGeneration;
	}
	if (m_hasOwner && m_ownerViewportId != view.viewportId)
	{
		m_retentionAvailable = false;
		m_unavailableReason = ViewportRenderProgressReason::SessionCapacity;
		m_lastReason = m_unavailableReason;
		return GetProgress();
	}
	if (m_hasOwner && m_ownerViewportId == view.viewportId && m_unavailableReason == ViewportRenderProgressReason::SessionCapacity)
	{
		m_unavailableReason = ViewportRenderProgressReason::None;
	}

	const bool resumed = m_suspended && m_hasIdentity;
	UpdateIdentity(view, scene, frame, sceneGeneration, resources);
	m_suspended = false;
	m_suspensionPending = false;
	if (m_unavailableReason != ViewportRenderProgressReason::None)
	{
		return GetProgress();
	}
	m_ownerViewportId = view.viewportId;
	m_hasOwner = true;
	if (resumed && m_hasIdentity && !m_resetVisible)
	{
		m_lastReason = ViewportRenderProgressReason::Resumed;
		m_lastCommitTime = std::chrono::steady_clock::now();
	}
	m_retentionAvailable = resources.CanRetain();

	m_uniformData = {};
	if (m_clearDisplay)
	{
		m_uniformData.WorkFlags = ReferencePathTracerUniformData::WorkFlagClearDisplay;
	}
	if (!m_paused && !m_pausePending && !m_pendingCommit && m_committedSamples < TargetSampleCount)
	{
		PrepareWork(view.renderExtent);
	}
	return GetProgress();
}

void ReferencePathTracerSession::RecordSubmission(RhiSubmissionToken token, ReferencePathTracerResources& resources) noexcept
{
	if (m_selected && resources.IsAllocated())
	{
		resources.RecordUse();
	}
	if (m_workPrepared)
	{
		if ((m_uniformData.WorkFlags & ReferencePathTracerUniformData::WorkFlagCommitPrefix) != 0u)
		{
			m_pendingCommit =
			    PendingCommit{.Submission = token, .ExecutionGeneration = m_executionGeneration, .Prefix = m_committedSamples + 1u};
			m_nextRow = 0u;
		}
		else
		{
			m_nextRow += m_preparedRowCount;
		}
	}
	m_workPrepared = false;
	m_clearDisplay = false;
	m_resetVisible = false;
	m_uniformData = {};
}

void ReferencePathTracerSession::Suspend(ReferencePathTracerResources& resources) noexcept
{
	if (m_suspended)
	{
		if (m_suspensionPending && !m_pendingCommit)
		{
			FinalizeSuspension(resources);
		}
		else if (m_hasIdentity && resources.IsAllocated() && !resources.CanRetain())
		{
			m_discardedSamples = m_committedSamples;
			m_committedSamples = 0u;
			m_lastReason = ViewportRenderProgressReason::RetentionReleased;
			resources.Release();
			m_hasIdentity = false;
			m_hasOwner = false;
			m_retentionAvailable = false;
		}
		return;
	}
	if (!m_hasIdentity && !resources.IsAllocated())
	{
		m_suspended = true;
		return;
	}
	m_suspended = true;
	m_workPrepared = false;
	m_nextRow = 0u;
	if (m_pendingCommit)
	{
		m_suspensionPending = true;
		return;
	}
	FinalizeSuspension(resources);
}

void ReferencePathTracerSession::FinalizeSuspension(ReferencePathTracerResources& resources) noexcept
{
	m_suspensionPending = false;
	++m_executionGeneration;
	if (!resources.CanRetain())
	{
		m_discardedSamples = m_committedSamples;
		m_committedSamples = 0u;
		m_lastReason = ViewportRenderProgressReason::RetentionReleased;
		resources.Release();
		m_hasIdentity = false;
		m_hasOwner = false;
		m_retentionAvailable = false;
	}
}

ViewportRenderProgress ReferencePathTracerSession::GetProgress() const noexcept
{
	const ViewportRenderProgressState state = m_unavailableReason != ViewportRenderProgressReason::None
	    ? ViewportRenderProgressState::Unavailable
	    : (m_paused ? ViewportRenderProgressState::Paused
	                : (m_resetVisible ? ViewportRenderProgressState::Resetting
	                                  : (m_committedSamples >= TargetSampleCount ? ViewportRenderProgressState::Complete
	                                                                             : ViewportRenderProgressState::Accumulating)));
	const double remainingSamples = static_cast<double>(TargetSampleCount - (std::min) (m_committedSamples, TargetSampleCount));
	const double estimatedSeconds =
	    m_samplesPerSecond > 0.0 && m_pixelCount > 0u ? (remainingSamples * static_cast<double>(m_pixelCount)) / m_samplesPerSecond : 0.0;
	return ViewportRenderProgress{
	    .State = state,
	    .Reason = m_unavailableReason != ViewportRenderProgressReason::None ? m_unavailableReason : m_lastReason,
	    .RequestedRoute = ViewportRenderProgressRoute::Automatic,
	    .ActiveRoute = m_unavailableReason == ViewportRenderProgressReason::None ? ViewportRenderProgressRoute::InlineRayTracing
	                                                                             : ViewportRenderProgressRoute::None,
	    .BackendApi = m_deviceServices.GetCapabilities().BackendApi,
	    .CompletedWork = m_committedSamples,
	    .TargetWork = TargetSampleCount,
	    .DiscardedWork = m_discardedSamples,
	    .OwnerViewportId = m_ownerViewportId,
	    .SamplesPerSecond = m_samplesPerSecond,
	    .EstimatedSecondsRemaining = estimatedSeconds,
	    .RetentionAvailable = m_retentionAvailable};
}
