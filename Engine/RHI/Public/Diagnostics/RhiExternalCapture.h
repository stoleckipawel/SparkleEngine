#pragma once

#include "../Core/RhiBackendApi.h"
#include "ExternalCaptureProvider.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <memory>
#include <string>

enum class ExternalCaptureState : std::uint8_t
{
	Unavailable,
	Ready,
	Queued,
	Capturing,
	Completed,
	Failed,
	Quarantined
};

constexpr bool CanRequestExternalCapture(ExternalCaptureState state) noexcept
{
	return state == ExternalCaptureState::Ready || state == ExternalCaptureState::Completed || state == ExternalCaptureState::Failed;
}

enum class ExternalCaptureAdmissionStatus : std::uint8_t
{
	Accepted,
	Busy,
	Unavailable,
	Full,
	Closed
};

struct ExternalCaptureAdmission final
{
	ExternalCaptureAdmissionStatus Status = ExternalCaptureAdmissionStatus::Unavailable;
	std::uint64_t RequestId = 0;
};

// Installed-tool observation only; does not load hooks or establish readiness.
SPARKLE_RHI_API bool HasInstalledExternalCaptureProvider() noexcept;

struct ExternalCaptureSnapshot final
{
	ExternalCaptureProvider Provider = ExternalCaptureProvider::None;
	ExternalCaptureState State = ExternalCaptureState::Unavailable;
	std::uint64_t RequestId = 0;
	std::uint64_t FrameId = 0;
	std::uint64_t ContextToken = 0;
	std::string ArtifactPath;
	std::string Message;
};

class RhiExternalCaptureBinding;
class RhiExternalCaptureState;

// Pre-device bootstrap and the native activity lease. The owner must outlive its
// device. SDK modules retain their process-lifetime hooks even after this owner dies.
class SPARKLE_RHI_API RhiExternalCapture final
{
public:
	RhiExternalCapture(ERhiBackendApi api, ExternalCaptureProvider provider);
	~RhiExternalCapture() noexcept;
	RhiExternalCapture(const RhiExternalCapture&) = delete;
	RhiExternalCapture& operator=(const RhiExternalCapture&) = delete;

	ExternalCaptureSnapshot Observe() const;
	ExternalCaptureAdmission Reserve(std::uint64_t contextToken);
	void Reject(std::uint64_t requestId, const char* reason) noexcept;
	void Arm(std::uint64_t requestId) noexcept;
	void BeginFrame(std::uint64_t frameId) noexcept;
	void EndFrame() noexcept;
	void InvalidateTarget() noexcept;
	void Shutdown() noexcept;

private:
	friend class RhiExternalCaptureBinding;
	void Bind(void* root, void* window) noexcept;
	std::unique_ptr<RhiExternalCaptureState> m_state;
};
