#pragma once

#include "Frame/Graph/BuildRenderFrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <memory>
#include <string_view>

class FrameGraph;
class FrameGraphBuilder;
class GpuMeshCache;
class IRayReconstructionProvider;
class IUpscalerProvider;
class RenderPassRuntimeCache;
class RenderHardwareInterface;
class RenderRayTracingScene;
class Window;

struct RenderFrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	const RenderPassRuntimeCache& renderPassRuntimeCache;
	GpuMeshCache& gpuMeshCache;
	RenderRayTracingScene& rayTracingScene;
	IUpscalerProvider* upscalerProvider = nullptr;
	IRayReconstructionProvider* rayReconstructionProvider = nullptr;
	Window& window;
	RenderFrameGraphSettings settings;
};

struct RenderFrameGraphBuildResult
{
	RenderFrameGraphBuildResult() noexcept;
	~RenderFrameGraphBuildResult() noexcept;

	RenderFrameGraphBuildResult(const RenderFrameGraphBuildResult&) = delete;
	RenderFrameGraphBuildResult& operator=(const RenderFrameGraphBuildResult&) = delete;
	RenderFrameGraphBuildResult(RenderFrameGraphBuildResult&&) noexcept;
	RenderFrameGraphBuildResult& operator=(RenderFrameGraphBuildResult&&) noexcept;

	std::unique_ptr<FrameGraph> Graph;
	RenderFrameGraphResources Resources = {};
};

class RenderFrameGraphFactory final
{
public:
	explicit RenderFrameGraphFactory(const RenderFrameGraphDependencies& dependencies) noexcept;

	RenderFrameGraphBuildResult Build() const;

private:
	static void ExportTextureIfValid(FrameGraphBuilder& builder, FrameGraphTextureHandle handle, std::string_view name) noexcept;
	static void ExportFrameProductRoots(
	    FrameGraphBuilder& builder,
	    const RenderFrameGraphSettings& settings,
	    const RenderFrameGraphResources& resources) noexcept;

	RenderFrameGraphDependencies m_dependencies;
};
