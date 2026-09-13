#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"

#include "Frame/RenderFrameTime.h"
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

bool ReferencePathTracerSession::Supports(const RenderView& view, const PreparedRenderScene& scene) const noexcept
{
	const RhiCapabilities& capabilities = m_deviceServices.GetCapabilities();
	const RhiFormatSupport* format = capabilities.FindFormatSupport(PixelFormat::R32G32B32A32_Float);
	const MaterialTextureTableCapabilityReport materialTextures = BuildMaterialTextureTableCapabilityReport(capabilities);
	const bool supportsView = view.camera.ProjectionKind == CameraProjectionKind::Perspective && view.renderExtent.IsValid()
	    && view.renderExtent.Width <= 16384u && view.renderExtent.Height <= 16384u;
	const bool supportsAccumulation = format != nullptr && format->SupportsShaderResource && format->SupportsUnorderedAccess;
	const bool supportsTracing = capabilities.RayTracing.SupportsAccelerationStructure && capabilities.RayTracing.SupportsInlineRayQuery;
	if (!supportsView || !supportsAccumulation || !supportsTracing || !materialTextures.Supported)
	{
		return false;
	}
	return std::none_of(
	    scene.materials.begin(),
	    scene.materials.end(),
	    [](const MaterialData& material) { return material.alphaMode == 2u || material.subsurfaceStrength != 0.0f; });
}

void ReferencePathTracerSession::BeginIdentity(
    const ReferencePathTracerIdentity& identity,
    ReferencePathTracerResetReason reason,
    double now) noexcept
{
	m_discardedSamples = m_committedSamples;
	m_lastReset = reason;
	m_identity = identity;
	m_hasIdentity = true;
	m_committedSamples = 0u;
	m_nextRow = 0u;
	m_pendingCommit = {};
	m_workPrepared = false;
	m_clearDisplay = true;
	m_suspensionPending = false;
	m_cancelled = false;
	m_timedOut = false;
	m_supportRejected = false;
	if (m_sessionStartSeconds == 0.0)
	{
		m_sessionStartSeconds = now;
	}
	m_firstCommitSeconds = 0.0;
	m_lastCommitSeconds = 0.0;
	++m_executionGeneration;
}

void ReferencePathTracerSession::UpdateIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    double now,
    ReferencePathTracerResources& resources) noexcept
{
	const ReferencePathTracerIdentity identity =
	    BuildReferencePathTracerIdentity(view, scene, frame, sceneGeneration, m_deviceServices.GetCapabilities().BackendApi);
	if (m_hasIdentity && identity == m_identity)
	{
		return;
	}
	if (!Supports(view, scene))
	{
		m_discardedSamples = m_committedSamples;
		m_committedSamples = 0u;
		m_pendingCommit = {};
		m_workPrepared = false;
		++m_executionGeneration;
		resources.Release();
		m_identity = identity;
		m_hasIdentity = true;
		m_supportRejected = true;
		m_cancelled = true;
		return;
	}
	const ReferencePathTracerResetReason reason =
	    m_hasIdentity ? ClassifyReferencePathTracerIdentityChange(m_identity, identity) : ReferencePathTracerResetReason::View;
	resources.Allocate(view.renderExtent);
	BeginIdentity(identity, reason, now);
}

void ReferencePathTracerSession::CompletePendingCommit(double now) noexcept
{
	if (!m_pendingCommit || !m_deviceServices.IsSubmissionComplete(m_pendingCommit.Submission))
	{
		return;
	}
	if (m_pendingCommit.ExecutionGeneration == m_executionGeneration)
	{
		m_committedSamples = m_pendingCommit.Prefix;
		m_lastCommitSeconds = now;
		if (m_firstCommitSeconds == 0.0)
		{
			m_firstCommitSeconds = now;
		}
	}
	m_pendingCommit = {};
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

ViewportRenderProgress ReferencePathTracerSession::Update(
    bool active,
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    const RenderFrameTime& time,
    std::uint64_t sceneGeneration,
    ReferencePathTracerResources& resources) noexcept
{
	const double now = time.UnscaledTime.count();
	m_lastUpdateSeconds = now;
	CompletePendingCommit(now);
	if (!active)
	{
		m_selected = false;
		Suspend(resources);
		return {};
	}
	m_selected = true;
	m_suspended = false;
	m_suspensionPending = false;
	if (m_requiresRestart)
	{
		return ProjectProgress();
	}

	UpdateIdentity(view, scene, frame, sceneGeneration, now, resources);
	if (m_supportRejected)
	{
		return ProjectProgress();
	}
	if (now - m_sessionStartSeconds >= std::chrono::duration<double>(SessionTimeout).count())
	{
		m_timedOut = true;
		m_workPrepared = false;
	}

	m_uniformData = {};
	if (m_clearDisplay)
	{
		m_uniformData.WorkFlags = ReferencePathTracerUniformData::WorkFlagClearDisplay;
	}
	if (!m_paused && !m_cancelled && !m_timedOut && !m_pendingCommit && m_committedSamples < m_targetSamples)
	{
		PrepareWork(view.renderExtent);
	}
	return ProjectProgress();
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
	m_uniformData = {};
}

void ReferencePathTracerSession::SetTargetSampleCount(std::uint32_t target) noexcept
{
	m_targetSamples = (std::clamp) (target, 1u, MaximumSampleCount);
}

void ReferencePathTracerSession::Pause() noexcept
{
	m_paused = true;
}

void ReferencePathTracerSession::Resume() noexcept
{
	m_paused = false;
}

void ReferencePathTracerSession::Restart(ReferencePathTracerResources& resources) noexcept
{
	const bool canRestartExistingPrefix = m_hasIdentity && resources.IsAllocated() && !m_supportRejected;
	m_requiresRestart = false;
	m_supportRejected = false;
	m_cancelled = false;
	m_timedOut = false;
	m_sessionStartSeconds = m_lastUpdateSeconds;
	if (canRestartExistingPrefix)
	{
		BeginIdentity(m_identity, ReferencePathTracerResetReason::Restart, m_lastUpdateSeconds);
	}
	else
	{
		m_hasIdentity = false;
	}
}

void ReferencePathTracerSession::Cancel(ReferencePathTracerResources& resources) noexcept
{
	m_discardedSamples = m_committedSamples;
	m_cancelled = true;
	m_requiresRestart = true;
	m_pendingCommit = {};
	m_workPrepared = false;
	m_suspensionPending = false;
	m_hasIdentity = false;
	resources.Release();
	++m_executionGeneration;
}

void ReferencePathTracerSession::Suspend(ReferencePathTracerResources& resources) noexcept
{
	if (m_suspended)
	{
		if (m_suspensionPending && !m_pendingCommit)
		{
			FinalizeSuspension(resources);
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
		m_cancelled = true;
		m_requiresRestart = true;
		resources.Release();
		m_hasIdentity = false;
	}
}

ReferencePathTracerProgress ReferencePathTracerSession::GetProgress() const noexcept
{
	double eta = -1.0;
	if (m_firstCommitSeconds > 0.0 && m_lastCommitSeconds > m_firstCommitSeconds && m_committedSamples > 1u)
	{
		const double secondsPerSample = (m_lastCommitSeconds - m_firstCommitSeconds) / static_cast<double>(m_committedSamples - 1u);
		eta = secondsPerSample * static_cast<double>(m_targetSamples > m_committedSamples ? m_targetSamples - m_committedSamples : 0u);
	}
	return ReferencePathTracerProgress{
	    .State = m_cancelled || m_timedOut          ? ViewportRenderProgressState::Unavailable
	        : m_committedSamples >= m_targetSamples ? ViewportRenderProgressState::Complete
	                                                : ViewportRenderProgressState::Rendering,
	    .CommittedSamples = m_committedSamples,
	    .TargetSamples = m_targetSamples,
	    .LastReset = m_lastReset,
	    .ActiveRoute = m_cancelled || m_timedOut ? ReferencePathTracerRoute::Unavailable : ReferencePathTracerRoute::InlineRayQuery,
	    .DiscardedSamples = m_discardedSamples,
	    .EstimatedSecondsRemaining = eta};
}

ViewportRenderProgress ReferencePathTracerSession::ProjectProgress() const noexcept
{
	const ReferencePathTracerProgress progress = GetProgress();
	return ViewportRenderProgress{
	    .State = progress.State,
	    .CompletedWork = progress.CommittedSamples,
	    .TargetWork = progress.TargetSamples};
}
