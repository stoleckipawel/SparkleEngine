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
class MaterialCache;
class PerViewDataBuilder;
class RenderPassRuntimeCache;
class RenderCamera;
class RendererBackendOwner;
class RenderDeviceServices;
class RenderHardwareInterface;
class RendererMemoryMonitor;
class RendererImageProviderStack;
class RenderRayTracingScene;
class RenderPreparationGraph;
class RhiImGuiRenderer;
class RenderWorld;
class TemporalDataBuilder;
class TextureCache;
class TaskExecutor;
class TaskScope;
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
	MaterialCache& GetMaterialCache() noexcept { return *m_materialCache; }
	RenderPreparationGraph& GetRenderPreparationGraph() noexcept { return *m_renderPreparationGraph; }
	RenderRayTracingScene* GetRenderRayTracingScene() noexcept { return m_renderRayTracingScene.get(); }
	const RenderRayTracingScene* GetRenderRayTracingScene() const noexcept { return m_renderRayTracingScene.get(); }
	PerViewDataBuilder& GetPerViewDataBuilder() noexcept { return *m_perViewDataBuilder; }
	TemporalDataBuilder& GetTemporalDataBuilder() noexcept { return *m_temporalDataBuilder; }
	RenderCamera& GetRenderCamera() noexcept { return *m_renderCamera; }
	RenderWorld& GetRenderWorld() noexcept { return *m_renderWorld; }
	const RenderWorld& GetRenderWorld() const noexcept { return *m_renderWorld; }
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
	std::unique_ptr<MaterialCache> m_materialCache;
	std::unique_ptr<RenderPreparationGraph> m_renderPreparationGraph;
	std::unique_ptr<RenderRayTracingScene> m_renderRayTracingScene;
	std::unique_ptr<PerViewDataBuilder> m_perViewDataBuilder;
	std::unique_ptr<TemporalDataBuilder> m_temporalDataBuilder;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<RenderWorld> m_renderWorld;
	std::unique_ptr<RendererImageProviderStack> m_imageProviders;
	std::uint64_t m_imageProviderGeneration = 1;
	struct RetiredImageProviderGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<RendererImageProviderStack> Providers;
	};
	std::vector<RetiredImageProviderGeneration> m_retiredImageProviders;
};
