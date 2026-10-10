#include "PCH.h"
#include "Diagnostics/RhiExternalCapture.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"
#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <Windows.h>
#include <chrono>
#include <format>
#include <mutex>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_externalCaptureLogger, "RHI.ExternalCapture");

static constexpr auto CaptureArmDeadline = std::chrono::seconds(5);
static constexpr auto CaptureFinalizationDeadline = std::chrono::seconds(120);

class RhiExternalCaptureState final
{
public:
	// Snapshot transitions are serialized here. Target, adapter and frame brackets
	// belong to the render consumer; no native call holds the publication mutex.
	mutable std::mutex Mutex;
	ExternalCaptureSnapshot Snapshot;
	std::unique_ptr<ExternalCaptureAdapter> Adapter;
	ExternalCaptureNativeTarget Target;
	std::chrono::steady_clock::time_point ReservedAt;
	std::chrono::steady_clock::time_point StartedAt;
	std::uint64_t NextRequestId = 1;
	std::uint64_t ArmedRequestId = 0;
	bool FrameStarted = false;
	bool Closed = false;

	void ExpireRequest(std::chrono::steady_clock::time_point now)
	{
		if (Snapshot.State == ExternalCaptureState::Queued && now - ReservedAt > CaptureArmDeadline)
		{
			Snapshot.State = ExternalCaptureState::Failed;
			Snapshot.Message = "No eligible frame arrived within the five-second arm deadline.";
		}
		if (Snapshot.State == ExternalCaptureState::Capturing && now - StartedAt > CaptureFinalizationDeadline)
		{
			Snapshot.State = ExternalCaptureState::Quarantined;
			Snapshot.Message = "Native finalization timed out. Restart the Editor; cancellation and overlapping capture are unsafe.";
		}
	}

	std::uint64_t ClaimArmedFrame(std::uint64_t frameId)
	{
		std::lock_guard lock(Mutex);
		ExpireRequest(std::chrono::steady_clock::now());
		if (Snapshot.State != ExternalCaptureState::Queued || ArmedRequestId != Snapshot.RequestId)
		{
			return 0;
		}

		// Claim before native Begin: an arm timeout cannot release a live native lease.
		ArmedRequestId = 0;
		StartedAt = std::chrono::steady_clock::now();
		Snapshot.State = ExternalCaptureState::Capturing;
		Snapshot.FrameId = frameId;
		Snapshot.Message = "Native capture pending; no artifact is confirmed yet.";
		return Snapshot.RequestId;
	}

	void PublishStartFailure(std::uint64_t requestId, const std::string& error)
	{
		std::lock_guard lock(Mutex);
		if (Snapshot.RequestId != requestId || Snapshot.State != ExternalCaptureState::Capturing)
		{
			return;
		}

		Snapshot.State = ExternalCaptureState::Failed;
		Snapshot.Message = error;
		SPDLOG_LOGGER_WARN(g_externalCaptureLogger.GetLogger(), "Capture request={} rejected: {}", requestId, error);
	}

	void PublishNativeResult(std::uint64_t requestId, const ExternalCaptureNativeResult& result)
	{
		std::lock_guard lock(Mutex);
		if (Snapshot.RequestId != requestId || Snapshot.State != ExternalCaptureState::Capturing)
		{
			return;
		}
		if (result.State == ExternalCaptureState::Capturing)
		{
			ExpireRequest(std::chrono::steady_clock::now());
			return;
		}

		Snapshot.State = result.State;
		Snapshot.ArtifactPath = result.Artifact.string();
		Snapshot.Message = result.Message;

		SPDLOG_LOGGER_INFO(
		    g_externalCaptureLogger.GetLogger(),
		    "Capture request={} state={} artifact={} {}",
		    requestId,
		    static_cast<int>(result.State),
		    Snapshot.ArtifactPath,
		    Snapshot.Message);
	}

	void Quarantine(std::uint64_t requestId, const char* reason)
	{
		std::lock_guard lock(Mutex);
		if (Snapshot.RequestId == requestId && Snapshot.State == ExternalCaptureState::Capturing)
		{
			Snapshot.State = ExternalCaptureState::Quarantined;
			Snapshot.Message = reason;
		}
	}
};

static std::filesystem::path CreateCaptureArtifactPath(std::uint64_t requestId, std::string& failure)
{
	const auto root = Filesystem::GetProductUserStatePaths().CapturesRoot / "ExternalCapture";
	std::error_code error;
	std::filesystem::create_directories(root, error);
	if (error)
	{
		failure = "Cannot create the capture output directory.";
		return {};
	}

	return root
	    / std::format("capture-{}-{}-{}", GetCurrentProcessId(), std::chrono::system_clock::now().time_since_epoch().count(), requestId);
}

RhiExternalCapture::RhiExternalCapture(ERhiBackendApi api, ExternalCaptureProvider provider) :

    m_state(std::make_unique<RhiExternalCaptureState>())
{
	auto& state = *m_state;
	state.Snapshot.Provider = provider;
	state.Adapter = CreateExternalCaptureAdapter(api, provider, state.Snapshot.Message);

	SPDLOG_LOGGER_INFO(
	    g_externalCaptureLogger.GetLogger(),
	    "Early capture bootstrap provider={} available={} {}",
	    static_cast<int>(provider),
	    state.Adapter != nullptr,
	    state.Snapshot.Message);
}

RhiExternalCapture::~RhiExternalCapture() noexcept = default;

void RhiExternalCapture::Bind(void* root, void* window) noexcept
{
	std::lock_guard lock(m_state->Mutex);
	m_state->Target = {root, window};
	if (m_state->Adapter != nullptr && root != nullptr && window != nullptr)
	{
		m_state->Snapshot.State = ExternalCaptureState::Ready;
		m_state->Snapshot.Message = "Capture the next host frame containing this viewport.";
	}
}

ExternalCaptureSnapshot RhiExternalCapture::Observe() const
{
	std::lock_guard lock(m_state->Mutex);
	m_state->ExpireRequest(std::chrono::steady_clock::now());
	return m_state->Snapshot;
}

ExternalCaptureAdmission RhiExternalCapture::Reserve(std::uint64_t contextToken)
{
	std::lock_guard lock(m_state->Mutex);
	if (m_state->Closed)
	{
		return {.Status = ExternalCaptureAdmissionStatus::Closed};
	}

	auto& snapshot = m_state->Snapshot;
	if (snapshot.State == ExternalCaptureState::Unavailable)
	{
		return {};
	}
	if (!CanRequestExternalCapture(snapshot.State))
	{
		return {.Status = ExternalCaptureAdmissionStatus::Busy};
	}
	if (m_state->NextRequestId == 0)
	{
		Diagnostics::Fatal(g_externalCaptureLogger, __FILE__, __LINE__, "External capture request identity exhausted.");
	}

	snapshot.RequestId = m_state->NextRequestId++;
	snapshot.FrameId = 0;
	snapshot.ContextToken = contextToken;
	snapshot.ArtifactPath.clear();
	snapshot.Message = "Capture request queued.";
	snapshot.State = ExternalCaptureState::Queued;
	m_state->ReservedAt = std::chrono::steady_clock::now();
	return {.Status = ExternalCaptureAdmissionStatus::Accepted, .RequestId = snapshot.RequestId};
}

void RhiExternalCapture::Reject(std::uint64_t requestId, const char* reason) noexcept
{
	std::lock_guard lock(m_state->Mutex);
	auto& snapshot = m_state->Snapshot;
	if (snapshot.RequestId == requestId && snapshot.State == ExternalCaptureState::Queued)
	{
		snapshot.State = ExternalCaptureState::Failed;
		snapshot.Message = reason;
	}
}

void RhiExternalCapture::Arm(std::uint64_t requestId) noexcept
{
	std::lock_guard lock(m_state->Mutex);
	if (m_state->Snapshot.RequestId == requestId && m_state->Snapshot.State == ExternalCaptureState::Queued)
	{
		m_state->ArmedRequestId = requestId;
	}
}

void RhiExternalCapture::BeginFrame(std::uint64_t frameId) noexcept
{
	auto& state = *m_state;
	const auto requestId = state.ClaimArmedFrame(frameId);
	if (requestId == 0)
	{
		return;
	}

	try
	{
		std::string failure;
		const auto artifact = CreateCaptureArtifactPath(requestId, failure);
		if (artifact.empty() || !state.Adapter->Begin(state.Target, artifact, failure))
		{
			state.PublishStartFailure(requestId, failure);
			return;
		}

		state.FrameStarted = true;
		SPDLOG_LOGGER_INFO(g_externalCaptureLogger.GetLogger(), "Capture request={} scheduled at host frame={}", requestId, frameId);
	}
	catch (const std::exception& error)
	{
		state.Quarantine(requestId, error.what());
	}
}

void RhiExternalCapture::EndFrame() noexcept
{
	auto& state = *m_state;
	if (state.FrameStarted)
	{
		state.Adapter->End(state.Target);
		state.FrameStarted = false;
	}

	const auto snapshot = Observe();
	if (snapshot.State != ExternalCaptureState::Capturing)
	{
		return;
	}

	try
	{
		state.PublishNativeResult(snapshot.RequestId, state.Adapter->Poll());
	}
	catch (const std::exception& error)
	{
		state.Quarantine(snapshot.RequestId, error.what());
	}
}

void RhiExternalCapture::InvalidateTarget() noexcept
{
	const auto snapshot = Observe();
	Reject(snapshot.RequestId, "The host presentation target changed before capture started.");
	if (snapshot.State == ExternalCaptureState::Capturing)
	{
		m_state->Quarantine(snapshot.RequestId, "Presentation changed during native capture. Restart before another capture.");
	}
}

void RhiExternalCapture::Shutdown() noexcept
{
	const auto snapshot = Observe();
	Reject(snapshot.RequestId, "Editor closed before capture started.");

	std::lock_guard lock(m_state->Mutex);
	m_state->Closed = true;
}
