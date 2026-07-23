#pragma once

#include "RendererAPI.h"
#include "Shaders/CookedShaderReloadResult.h"
#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Diagnostics/MeshPreviewGeometry.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"
#include "Rendering/RenderInputFrame.h"
#include "Concurrency/RendererExecutionConfig.h"
#include "RendererSerialUiCallback.h"

#include <cstdint>
#include <memory>

class Timer;
class RhiImGuiRenderer;
class Window;
class RendererFacadeState;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, Window& window, RendererExecutionConfig config = {}) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept;
	void SubmitRenderInput(RenderInputFrame input) noexcept;

	ViewportRenderProducts GetViewportRenderProducts() const;

	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	CookedShaderReloadResult ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId) const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics() const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void RenderSerialUiFrame(RendererSerialUiCallback composeUi, void* context) noexcept;
	void WaitForIdle() noexcept;
	void BeginHostPresentation(const float clearColor[4]) noexcept;
	void BeginHostOverlayPresentation() noexcept;
	void EndHostPresentation() noexcept;
	ViewportPresentationProduct BeginViewportPresentation(RenderOutputFlags output) noexcept;
	void EndViewportPresentation(RenderOutputFlags output) noexcept;
	ViewportCaptureResult CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept;

	void OnRender() noexcept;

  private:
	std::unique_ptr<RendererFacadeState> m_state;
};
