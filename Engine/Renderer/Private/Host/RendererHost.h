#pragma once

#include <cstdint>
#include <memory>

class FramePipeline;
class RenderPassRuntimeCache;
class RendererBackendOwner;
class RendererMemoryMonitor;
class RendererExecutionContext;
class TaskExecutor;
class TaskScope;
class Window;
struct RendererMemoryDiagnosticsSnapshot;
struct RendererBackendConfiguration;

class RendererHost final
{
public:
	RendererHost(Window& window, const RendererBackendConfiguration& backendConfiguration) noexcept;
	~RendererHost() noexcept;

	RendererHost(const RendererHost&) = delete;
	RendererHost& operator=(const RendererHost&) = delete;
	RendererHost(RendererHost&&) = delete;
	RendererHost& operator=(RendererHost&&) = delete;

private:
	friend class RendererExecutionContext;

	std::unique_ptr<FramePipeline> CreateFramePipeline(
	    TaskExecutor& taskExecutor,
	    TaskScope& assetTaskParentScope,
	    bool enableUiRenderPackets) noexcept;
	void ReloadShaders();
	std::uint64_t GetShaderGeneration() const noexcept;
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	void SettleForShutdown() noexcept;

	Window& m_window;
	std::unique_ptr<RendererBackendOwner> m_backendOwner;
	std::unique_ptr<RenderPassRuntimeCache> m_renderPassRuntimeCache;
	std::unique_ptr<RendererMemoryMonitor> m_memoryMonitor;
};
