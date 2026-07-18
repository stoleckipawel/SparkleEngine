#pragma once

#include "RendererAPI.h"
#include "Shaders/CookedShaderReloadResult.h"
#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>

class Timer;
class RhiImGuiRenderer;
class LevelManager;
class GameScene;
class Window;
class RendererState;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept;

	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept;

	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	CookedShaderReloadResult ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics() const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void PrepareHostFrame() noexcept;
	void RecordHostFrame() noexcept;
	void SubmitHostFrame() noexcept;
	void WaitForIdle() noexcept;
	void BeginHostPresentation(const float clearColor[4]) noexcept;
	void BeginHostOverlayPresentation() noexcept;
	void EndHostPresentation() noexcept;
	ViewportPresentationProduct BeginViewportPresentation(RenderOutputFlags output) noexcept;
	void EndViewportPresentation(RenderOutputFlags output) noexcept;
	ViewportCaptureResult CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept;

	void OnRender() noexcept;

  private:
	std::unique_ptr<RendererState> m_state;
};
