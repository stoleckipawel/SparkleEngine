#pragma once

#include "ReferencePathTracerIdentity.h"
#include "ReferencePathTracerUniformData.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RayTracing/RayTracingExecutionFrontend.h"
#include "RHI/Public/Commands/RhiQueue.h"

#include <chrono>
#include <cstdint>

class ReferencePathTracerResources;
class RenderDeviceServices;
struct PreparedRenderScene;
struct RenderFrameIdentity;
struct RenderView;

class ReferencePathTracerSession final
{
public:
	static constexpr std::uint32_t WorkRowsPerDispatch = 32u;

	explicit ReferencePathTracerSession(RenderDeviceServices& deviceServices) noexcept;

	ViewportRenderProgress Update(
	    const ViewportRenderRequest& request,
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration,
	    RayTracingExecutionFrontend executionFrontend,
	    ReferencePathTracerResources& resources) noexcept;
	void RecordSubmission(RhiSubmissionToken token, ReferencePathTracerResources& resources) noexcept;

	bool IsSelected() const noexcept { return m_selected; }
	bool CanBindResources() const noexcept { return m_unavailableReason == ViewportRenderProgressReason::None; }
	const ReferencePathTracerUniformData& GetUniformData() const noexcept { return m_uniformData; }

private:
	static constexpr std::uint32_t TargetSampleCount = 4096u;

	struct PendingCommit final
	{
		RhiSubmissionToken Submission = {};
		std::uint64_t ExecutionGeneration = 0u;
		std::uint32_t Prefix = 0u;

		explicit operator bool() const noexcept { return Submission.IsValid(); }
	};

	ViewportRenderProgressReason ResolveAvailability(
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    RayTracingExecutionFrontend executionFrontend) const noexcept;
	void BeginIdentity(
	    const ReferencePathTracerIdentity& identity,
	    RenderViewportExtent extent,
	    ViewportRenderProgressReason reason,
	    std::uint32_t discardedSamples) noexcept;
	void UpdateIdentity(
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration,
	    RayTracingExecutionFrontend executionFrontend,
	    ReferencePathTracerResources& resources) noexcept;
	void CompletePendingCommit() noexcept;
	void PrepareWork(RenderViewportExtent extent) noexcept;
	void ApplyAction(const ViewportRenderRequest& request, const RenderView& view, ReferencePathTracerResources& resources) noexcept;
	void Suspend(ReferencePathTracerResources& resources) noexcept;
	void FinalizeSuspension(ReferencePathTracerResources& resources) noexcept;
	ViewportRenderProgress GetProgress() const noexcept;

	RenderDeviceServices& m_deviceServices;
	ReferencePathTracerUniformData m_uniformData = {};
	ReferencePathTracerIdentity m_identity = {};
	RayTracingExecutionFrontend m_executionFrontend = RayTracingExecutionFrontend::None;
	PendingCommit m_pendingCommit = {};
	std::uint64_t m_executionGeneration = 0u;
	std::uint64_t m_ownerViewportId = 0u;
	std::uint64_t m_lastActionViewportId = 0u;
	std::uint64_t m_lastActionSequence = 0u;
	std::uint64_t m_transferViewportId = 0u;
	std::uint32_t m_committedSamples = 0u;
	std::uint32_t m_discardedSamples = 0u;
	std::uint64_t m_pixelCount = 0u;
	std::uint32_t m_nextRow = 0u;
	std::uint32_t m_preparedRowCount = 0u;
	double m_samplesPerSecond = 0.0;
	std::chrono::steady_clock::time_point m_lastCommitTime = {};
	ViewportRenderProgressReason m_lastReason = ViewportRenderProgressReason::None;
	ViewportRenderProgressReason m_unavailableReason = ViewportRenderProgressReason::None;
	bool m_hasOwner = false;
	bool m_transferPending = false;
	bool m_hasIdentity = false;
	bool m_workPrepared = false;
	bool m_clearDisplay = false;
	bool m_resetVisible = false;
	bool m_paused = false;
	bool m_pausePending = false;
	bool m_retentionAvailable = false;
	bool m_suspended = false;
	bool m_selected = false;
	bool m_suspensionPending = false;
};
