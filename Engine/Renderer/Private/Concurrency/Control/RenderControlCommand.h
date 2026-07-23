#pragma once

#include "Concurrency/Control/RenderControlCompletion.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"

#include <cstdint>
#include <memory>
#include <variant>

struct RenderFrameReadyCommand final
{
	RenderFrameQueueTicket Ticket;
};

struct RenderResizeCommand final
{
	RenderViewportExtent Extent;
	bool Minimized = false;
};

struct RenderViewportCommand final
{
	ViewportRenderRequest Request;
};

struct RenderReloadShadersCommand final
{
	std::shared_ptr<RenderControlCompletion> Completion;
};

struct RenderWaitForIdleCommand final
{
	std::shared_ptr<RenderControlCompletion> Completion;
};

enum class RenderDiagnosticsRequestKind : std::uint8_t
{
	Meshes,
	MeshPreview,
	Textures,
	Memory,
};

struct RenderDiagnosticsCommand final
{
	RenderDiagnosticsRequestKind Kind = RenderDiagnosticsRequestKind::Meshes;
	std::uintptr_t MeshRuntimeId = 0;
	std::shared_ptr<RenderControlCompletion> Completion;
};

struct RenderCaptureCommand final
{
	ViewportCaptureRequest Request;
	std::shared_ptr<RenderControlCompletion> Completion;
};

struct RenderRefreshProvidersCommand final
{
};

struct RenderSettingsChangedCommand final
{
};

struct RenderShutdownCommand final
{
};

using RenderControlPayload = std::variant<
    RenderFrameReadyCommand,
    RenderResizeCommand,
    RenderViewportCommand,
    RenderReloadShadersCommand,
    RenderWaitForIdleCommand,
    RenderDiagnosticsCommand,
    RenderCaptureCommand,
    RenderRefreshProvidersCommand,
    RenderSettingsChangedCommand,
    RenderShutdownCommand>;

struct RenderControlCommand final
{
	std::uint64_t SequenceNumber = 0;
	RenderControlPayload Payload;
};
