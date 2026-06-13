#pragma once

#include "RendererAPI.h"
#include "Shaders/CookedShaderReloadResult.h"
#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Diagnostics/RendererSmokeDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"
#include "../../RHI/Public/Device/RenderHardwareInterface.h"
#include "../../RHI/Public/Interop/ResourceState.h"
#include "../../RHI/Public/Interop/RhiNativeHandles.h"

#include <cstdint>
#include <memory>

class Timer;
class RhiImGuiRenderer;
class LevelManager;
class GameScene;
class Window;
class FramePipeline;
class RendererSystemRoot;

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

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	CookedShaderReloadResult ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics() const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	RendererSmokeDiagnosticsSnapshot CaptureSmokeDiagnostics() const;
	void PrepareHostFrame() noexcept;
	void RecordHostFrame() noexcept;
	void SubmitHostFrame() noexcept;
	std::uint64_t ResolveRenderProductTextureId(RenderProductHandle handle) noexcept;
	NativeResourceHandle ResolveRenderProductResource(RenderProductHandle handle) const noexcept;
	void TransitionRenderProduct(RenderProductHandle handle, ResourceState before, ResourceState after) noexcept;

	void OnRender() noexcept;

  private:
	std::unique_ptr<RendererSystemRoot> m_systemRoot;
	std::unique_ptr<FramePipeline> m_framePipeline;
};
