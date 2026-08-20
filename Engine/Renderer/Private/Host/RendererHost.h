#pragma once

#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Diagnostics/MeshPreviewGeometry.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "RHI/Public/Commands/RhiQueue.h"

#include <cstdint>
#include <memory>
#include <vector>

class FrameExecutionDiagnostics;
class GpuMeshCache;
class RenderPassRuntimeCache;
class RendererBackendOwner;
class RenderDeviceServices;
class RenderHardwareInterface;
class RendererMemoryMonitor;
class RendererImageProviderStack;
class RenderScenePreparation;
class RhiImGuiRenderer;
class RenderScene;
class TextureCache;
class TaskExecutor;
class TaskScope;
class RenderViewBuilder;
class RenderViewPreparation;
class RenderViewState;
class Window;
struct RendererBackendConfiguration;

class RendererHost final
{
public:
	RendererHost(
	    Window& window,
	    const RendererBackendConfiguration& backendConfiguration,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationTaskScope) noexcept;
	~RendererHost() noexcept;

	RendererHost(const RendererHost&) = delete;
	RendererHost& operator=(const RendererHost&) = delete;
	RendererHost(RendererHost&&) = delete;
	RendererHost& operator=(RendererHost&&) = delete;

	Window& GetWindow() noexcept { return *m_window; }
	const Window& GetWindow() const noexcept { return *m_window; }

	RenderDeviceServices& GetDeviceServices() noexcept;
	const RenderDeviceServices& GetDeviceServices() const noexcept;
	bool HasDeviceServices() const noexcept { return m_backendOwner != nullptr; }
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;

	RenderPassRuntimeCache& GetRenderPassRuntimeCache() noexcept { return *m_renderPassRuntimeCache; }
	const RenderPassRuntimeCache& GetRenderPassRuntimeCache() const noexcept { return *m_renderPassRuntimeCache; }
	GpuMeshCache& GetGpuMeshCache() noexcept { return *m_gpuMeshCache; }
	TextureCache& GetTextureCache() noexcept { return *m_textureCache; }
	RenderScenePreparation& GetRenderScenePreparation() noexcept { return *m_renderScenePreparation; }
	RenderViewBuilder& GetRenderViewBuilder() noexcept { return *m_renderViewBuilder; }
	RenderViewPreparation& GetRenderViewPreparation() noexcept { return *m_renderViewPreparation; }
	RenderViewState& GetRenderViewState() noexcept { return *m_renderViewState; }
	RenderScene& GetRenderScene() noexcept { return *m_renderScene; }
	const RenderScene& GetRenderScene() const noexcept { return *m_renderScene; }
	RendererImageProviderStack& GetImageProviders() noexcept { return *m_imageProviders; }
	const RendererImageProviderStack& GetImageProviders() const noexcept { return *m_imageProviders; }
	std::uint64_t GetImageProviderGeneration() const noexcept { return m_imageProviderGeneration; }
	TaskExecutor& GetTaskExecutor() noexcept { return *m_taskExecutor; }

	void ReloadCookedShaders();
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId) const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics(const TexturePreviewHandleResolver& resolvePreviewTexture) const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void TickDiagnostics(std::uint64_t frameIndex) noexcept;
	void RefreshImageProviders() noexcept;
	void PollRetiredImageProviders() noexcept;

private:
	void InitializeCoreRuntime(
	    const RendererBackendConfiguration& backendConfiguration,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationTaskScope) noexcept;
	void InitializeSceneRuntime(TaskExecutor& taskExecutor, TaskScope& applicationTaskScope) noexcept;

	Window* m_window = nullptr;
	TaskExecutor* m_taskExecutor = nullptr;

	std::unique_ptr<RendererBackendOwner> m_backendOwner;
	std::unique_ptr<RenderPassRuntimeCache> m_renderPassRuntimeCache;
	std::unique_ptr<RendererMemoryMonitor> m_memoryMonitor;
	std::unique_ptr<GpuMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureCache> m_textureCache;
	std::unique_ptr<RenderScenePreparation> m_renderScenePreparation;
	std::unique_ptr<RenderViewBuilder> m_renderViewBuilder;
	std::unique_ptr<RenderViewPreparation> m_renderViewPreparation;
	std::unique_ptr<RenderViewState> m_renderViewState;
	std::unique_ptr<RenderScene> m_renderScene;
	std::unique_ptr<RendererImageProviderStack> m_imageProviders;
	std::uint64_t m_imageProviderGeneration = 1;
	struct RetiredImageProviderGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<RendererImageProviderStack> Providers;
	};
	std::vector<RetiredImageProviderGeneration> m_retiredImageProviders;
};
