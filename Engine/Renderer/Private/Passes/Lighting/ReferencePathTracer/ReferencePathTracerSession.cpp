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

void ReferencePathTracerSession::BeginIdentity(const ReferencePathTracerIdentity& identity) noexcept
{
	m_identity = identity;
	m_hasIdentity = true;
	m_committedSamples = 0u;
	m_nextRow = 0u;
	m_pendingCommit = {};
	m_workPrepared = false;
	m_clearDisplay = true;
	m_suspensionPending = false;
	m_supportRejected = false;
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
	if (!Supports(view, scene))
	{
		m_committedSamples = 0u;
		m_pendingCommit = {};
		m_workPrepared = false;
		++m_executionGeneration;
		resources.Release();
		m_identity = identity;
		m_hasIdentity = true;
		m_supportRejected = true;
		return;
	}
	resources.Allocate(view.renderExtent);
	BeginIdentity(identity);
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
    std::uint64_t sceneGeneration,
    ReferencePathTracerResources& resources) noexcept
{
	CompletePendingCommit();
	if (!active)
	{
		m_selected = false;
		Suspend(resources);
		return {};
	}
	m_selected = true;
	m_suspended = false;
	m_suspensionPending = false;

	UpdateIdentity(view, scene, frame, sceneGeneration, resources);
	if (m_supportRejected)
	{
		return GetProgress();
	}

	m_uniformData = {};
	if (m_clearDisplay)
	{
		m_uniformData.WorkFlags = ReferencePathTracerUniformData::WorkFlagClearDisplay;
	}
	if (!m_pendingCommit && m_committedSamples < TargetSampleCount)
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
		resources.Release();
		m_hasIdentity = false;
	}
}

ViewportRenderProgress ReferencePathTracerSession::GetProgress() const noexcept
{
	const ViewportRenderProgressState state = m_supportRejected
	    ? ViewportRenderProgressState::Unavailable
	    : (m_committedSamples >= TargetSampleCount ? ViewportRenderProgressState::Complete : ViewportRenderProgressState::Rendering);
	return ViewportRenderProgress{
	    .State = state,
	    .CompletedWork = m_committedSamples,
	    .TargetWork = TargetSampleCount};
}
