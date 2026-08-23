#pragma once

#include <memory>

class GpuMeshCache;
class RenderPassRuntimeCache;
class RendererBackendOwner;
class RenderDeviceServices;
class RendererMemoryMonitor;
class RendererImageProviderStack;
class RenderScene;
class RenderScenePreparation;
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
	TaskExecutor& GetTaskExecutor() noexcept { return *m_taskExecutor; }
	RendererMemoryMonitor& GetMemoryMonitor() noexcept { return *m_memoryMonitor; }

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
};
