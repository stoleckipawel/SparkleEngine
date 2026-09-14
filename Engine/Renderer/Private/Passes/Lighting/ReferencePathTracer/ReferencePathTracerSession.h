#pragma once

#include "ReferencePathTracerIdentity.h"
#include "ReferencePathTracerUniformData.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Commands/RhiQueue.h"

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
	    bool active,
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration,
	    ReferencePathTracerResources& resources) noexcept;
	void RecordSubmission(RhiSubmissionToken token, ReferencePathTracerResources& resources) noexcept;

	bool IsSelected() const noexcept { return m_selected; }
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

	bool Supports(const RenderView& view, const PreparedRenderScene& scene) const noexcept;
	void BeginIdentity(const ReferencePathTracerIdentity& identity) noexcept;
	void UpdateIdentity(
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration,
	    ReferencePathTracerResources& resources) noexcept;
	void CompletePendingCommit() noexcept;
	void PrepareWork(RenderViewportExtent extent) noexcept;
	void Suspend(ReferencePathTracerResources& resources) noexcept;
	void FinalizeSuspension(ReferencePathTracerResources& resources) noexcept;
	ViewportRenderProgress GetProgress() const noexcept;

	RenderDeviceServices& m_deviceServices;
	ReferencePathTracerUniformData m_uniformData = {};
	ReferencePathTracerIdentity m_identity = {};
	PendingCommit m_pendingCommit = {};
	std::uint64_t m_executionGeneration = 0u;
	std::uint32_t m_committedSamples = 0u;
	std::uint32_t m_nextRow = 0u;
	std::uint32_t m_preparedRowCount = 0u;
	bool m_hasIdentity = false;
	bool m_workPrepared = false;
	bool m_clearDisplay = false;
	bool m_suspended = false;
	bool m_selected = false;
	bool m_supportRejected = false;
	bool m_suspensionPending = false;
};
