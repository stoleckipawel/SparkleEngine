#pragma once

#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Diagnostics/MeshPreviewGeometry.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "Shaders/CookedShaderReloadResult.h"

#include <cstdint>
#include <memory>
#include <vector>

class FrameExecutionDiagnostics;
class GPUMeshCache;
class MaterialCacheManager;
class PerViewDataBuilder;
class PipelineStateManager;
class RenderCamera;
class RendererBackendSystem;
class RenderDeviceServices;
class RenderHardwareInterface;
class RendererMemoryMonitor;
class RendererImageProviderStack;
class RenderRayTracingScene;
class RenderPreparationGraph;
class RhiImGuiRenderer;
class RenderWorld;
class TemporalDataBuilder;
class TextureManager;
class TaskExecutor;
class TaskScope;
class Window;
struct RendererBackendConfiguration;

class RendererSystemRoot final
{
  public:
	RendererSystemRoot(
	    Window& window,
	    const RendererBackendConfiguration& backendConfiguration,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationTaskScope) noexcept;
	~RendererSystemRoot() noexcept;

	RendererSystemRoot(const RendererSystemRoot&) = delete;
	RendererSystemRoot& operator=(const RendererSystemRoot&) = delete;
	RendererSystemRoot(RendererSystemRoot&&) = delete;
	RendererSystemRoot& operator=(RendererSystemRoot&&) = delete;

	Window& GetWindow() noexcept { return *m_window; }
	const Window& GetWindow() const noexcept { return *m_window; }

	RenderDeviceServices& GetBackend() noexcept;
	const RenderDeviceServices& GetBackend() const noexcept;
	bool HasBackend() const noexcept { return m_backend != nullptr; }
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;

	PipelineStateManager& GetPipelineStateManager() noexcept { return *m_pipelineStateManager; }
	const PipelineStateManager& GetPipelineStateManager() const noexcept { return *m_pipelineStateManager; }
	GPUMeshCache& GetGpuMeshCache() noexcept { return *m_gpuMeshCache; }
	TextureManager& GetTextureManager() noexcept { return *m_textureManager; }
	MaterialCacheManager& GetMaterialCacheManager() noexcept { return *m_materialCacheManager; }
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
	TaskExecutor& GetTaskExecutor() noexcept { return *m_taskExecutor; }

	CookedShaderReloadResult ReloadCookedShaders() noexcept;
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId) const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics(
	    const TexturePreviewHandleResolver& resolvePreviewTexture) const;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void TickDiagnostics(std::uint64_t frameIndex) noexcept;
	void PostLoad() noexcept;
	void RefreshImageProviders() noexcept;
	void PollRetiredImageProviders() noexcept;

  private:
	void InitializeCoreSystems(const RendererBackendConfiguration& backendConfiguration) noexcept;
	void InitializeSceneSystems(
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationTaskScope) noexcept;

	Window* m_window = nullptr;
	TaskExecutor* m_taskExecutor = nullptr;

	std::unique_ptr<RendererBackendSystem> m_backend;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<RendererMemoryMonitor> m_memoryMonitor;
	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureManager> m_textureManager;
	std::unique_ptr<MaterialCacheManager> m_materialCacheManager;
	std::unique_ptr<RenderPreparationGraph> m_renderPreparationGraph;
	std::unique_ptr<RenderRayTracingScene> m_renderRayTracingScene;
	std::unique_ptr<PerViewDataBuilder> m_perViewDataBuilder;
	std::unique_ptr<TemporalDataBuilder> m_temporalDataBuilder;
	std::unique_ptr<RenderCamera> m_renderCamera;
	std::unique_ptr<RenderWorld> m_renderWorld;
	std::unique_ptr<RendererImageProviderStack> m_imageProviders;
	struct RetiredImageProviderGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<RendererImageProviderStack> Providers;
	};
	std::vector<RetiredImageProviderGeneration> m_retiredImageProviders;
};
