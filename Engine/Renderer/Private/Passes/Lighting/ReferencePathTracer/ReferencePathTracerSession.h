#pragma once

#include "ReferencePathTracerIdentity.h"
#include "ReferencePathTracerUniformData.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Commands/RhiQueue.h"

#include <chrono>
#include <cstdint>

class ReferencePathTracerResources;
class RenderDeviceServices;
struct PreparedRenderScene;
struct RenderFrameIdentity;
struct RenderFrameTime;
struct RenderView;

enum class ReferencePathTracerRoute : std::uint8_t
{
	Unavailable,
	InlineRayQuery,
};

struct ReferencePathTracerProgress final
{
	ViewportRenderProgressState State = ViewportRenderProgressState::None;
	std::uint32_t CommittedSamples = 0u;
	std::uint32_t TargetSamples = 0u;
	ReferencePathTracerResetReason LastReset = ReferencePathTracerResetReason::None;
	ReferencePathTracerRoute ActiveRoute = ReferencePathTracerRoute::Unavailable;
	std::uint32_t DiscardedSamples = 0u;
	double EstimatedSecondsRemaining = -1.0;
};

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
	    const RenderFrameTime& time,
	    std::uint64_t sceneGeneration,
	    ReferencePathTracerResources& resources) noexcept;
	void RecordSubmission(RhiSubmissionToken token, ReferencePathTracerResources& resources) noexcept;

	void SetTargetSampleCount(std::uint32_t target) noexcept;
	void Pause() noexcept;
	void Resume() noexcept;
	void Restart(ReferencePathTracerResources& resources) noexcept;
	void Cancel(ReferencePathTracerResources& resources) noexcept;

	bool IsSelected() const noexcept { return m_selected; }
	const ReferencePathTracerUniformData& GetUniformData() const noexcept { return m_uniformData; }
	ReferencePathTracerProgress GetProgress() const noexcept;

private:
	static constexpr std::uint32_t MaximumSampleCount = 1'048'576u;
	static constexpr std::chrono::hours SessionTimeout{8};

	struct PendingCommit final
	{
		RhiSubmissionToken Submission = {};
		std::uint64_t ExecutionGeneration = 0u;
		std::uint32_t Prefix = 0u;

		explicit operator bool() const noexcept { return Submission.IsValid(); }
	};

	bool Supports(const RenderView& view, const PreparedRenderScene& scene) const noexcept;
	void BeginIdentity(const ReferencePathTracerIdentity& identity, ReferencePathTracerResetReason reason, double now) noexcept;
	void UpdateIdentity(
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration,
	    double now,
	    ReferencePathTracerResources& resources) noexcept;
	void CompletePendingCommit(double now) noexcept;
	void PrepareWork(RenderViewportExtent extent) noexcept;
	void Suspend(ReferencePathTracerResources& resources) noexcept;
	void FinalizeSuspension(ReferencePathTracerResources& resources) noexcept;
	ViewportRenderProgress ProjectProgress() const noexcept;

	RenderDeviceServices& m_deviceServices;
	ReferencePathTracerUniformData m_uniformData = {};
	ReferencePathTracerIdentity m_identity = {};
	PendingCommit m_pendingCommit = {};
	std::uint64_t m_executionGeneration = 0u;
	std::uint32_t m_committedSamples = 0u;
	std::uint32_t m_targetSamples = 4096u;
	std::uint32_t m_nextRow = 0u;
	std::uint32_t m_preparedRowCount = 0u;
	std::uint32_t m_discardedSamples = 0u;
	ReferencePathTracerResetReason m_lastReset = ReferencePathTracerResetReason::None;
	double m_sessionStartSeconds = 0.0;
	double m_firstCommitSeconds = 0.0;
	double m_lastCommitSeconds = 0.0;
	double m_lastUpdateSeconds = 0.0;
	bool m_hasIdentity = false;
	bool m_workPrepared = false;
	bool m_clearDisplay = false;
	bool m_paused = false;
	bool m_suspended = false;
	bool m_cancelled = false;
	bool m_timedOut = false;
	bool m_selected = false;
	bool m_supportRejected = false;
	bool m_requiresRestart = false;
	bool m_suspensionPending = false;
};
