#pragma once

#include "Concurrency/Control/RenderControlCompletion.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <variant>

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
	ViewportCaptureId Id;
	ViewportCaptureRequest Request;
};

struct RenderSettingsChangedCommand final
{
	EngineRenderingSettingsState Settings;
};

struct RenderShutdownCommand final
{
};

using RendererExecutionControl = std::variant<
    RenderResizeCommand,
    RenderViewportCommand,
    RenderReloadShadersCommand,
    RenderDiagnosticsCommand,
    RenderCaptureCommand,
    RenderSettingsChangedCommand,
    RenderShutdownCommand>;
