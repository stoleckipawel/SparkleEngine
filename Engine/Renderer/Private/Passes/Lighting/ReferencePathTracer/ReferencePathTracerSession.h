#pragma once

#include "ReferencePathTracerIdentity.h"
#include "ReferencePathTracerResources.h"
#include "ReferencePathTracerUniformData.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RayTracing/RayTracingExecutionFrontend.h"
#include "RHI/Public/Commands/RhiQueue.h"

#include <chrono>
#include <cstdint>

class FrameGraph;
class FrameGraphBuilder;
class RendererMemoryMonitor;
class RenderDeviceServices;
class RenderRayTracingScene;
struct PreparedRenderScene;
struct RenderFrame;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;
struct RenderView;
struct ViewportFrameProducts;

class ReferencePathTracerSession final
{
public:
	ReferencePathTracerSession(
	    RenderDeviceServices& deviceServices,
	    RendererMemoryMonitor& memoryMonitor,
	    RenderRayTracingScene& rayTracingScene) noexcept;

	bool PrepareFrame(
	    const RenderFrame& frame,
	    ViewportRenderAction action,
	    std::uint64_t actionSequence,
	    ViewportFrameProducts& products,
	    FrameGraph& frameGraph) noexcept;
	void OnFrameSubmitted(RhiSubmissionToken token) noexcept;

private:
	friend void AddReferencePathTracerPasses(
	    FrameGraphBuilder& builder,
	    const RenderFrameGraphSettings& settings,
	    ReferencePathTracerSession& session,
	    RenderFrameGraphResources& resources);
	friend void AddReferencePathTracerTransportPass(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent extent,
	    const RenderFrameGraphResources& resources,
	    const ReferencePathTracerGraphResources& graphResources,
	    const ReferencePathTracerUniformData& uniformData,
	    RenderRayTracingScene& rayTracingScene);

	static constexpr std::uint32_t WorkRowsPerDispatch = 32u;

	ViewportRenderProgress Update(
	    const RenderFrame& frame,
	    ViewportRenderAction action,
	    std::uint64_t actionSequence,
	    RayTracingExecutionFrontend executionFrontend) noexcept;
	void ReserveGraphResources(FrameGraphBuilder& builder, RenderViewportExtent extent);
	const ReferencePathTracerGraphResources& GetGraphResources() const noexcept { return m_resources.GetGraphResources(); }
	bool BindResources(FrameGraph& frameGraph) const noexcept;

	const ReferencePathTracerUniformData& GetUniformData() const noexcept { return m_uniformData; }

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
	void UpdateIdentity(const RenderFrame& frame, RayTracingExecutionFrontend executionFrontend) noexcept;
	void CompletePendingCommit() noexcept;
	void PrepareWork(RenderViewportExtent extent) noexcept;
	void ApplyAction(ViewportRenderAction action, std::uint64_t actionSequence, const RenderView& view) noexcept;
	void Suspend() noexcept;
	void FinalizeSuspension() noexcept;
	ViewportRenderProgress GetProgress() const noexcept;
	RenderProductSamplePrefix GetRadianceSamplePrefix() const noexcept;

	RenderDeviceServices& m_deviceServices;
	RenderRayTracingScene& m_rayTracingScene;
	ReferencePathTracerResources m_resources;
	ReferencePathTracerUniformData m_uniformData = {};
	ReferencePathTracerIdentity m_identity = {};
	Hash::Sha256Digest m_identitySha256 = {};
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
