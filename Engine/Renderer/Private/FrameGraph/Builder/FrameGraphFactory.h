#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <memory>
#include <string_view>

class FrameGraph;
class FrameGraphBuilder;
class PipelineStateManager;
class RenderHardwareInterface;
class Window;

struct FrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	const PipelineStateManager& pipelineStateManager;
	Window& window;
	RenderViewportExtent renderExtent;
	RenderViewportExtent outputExtent;
	bool presentSceneToBackBuffer = true;
};

struct FrameGraphBuildResult
{
	FrameGraphBuildResult() noexcept;
	~FrameGraphBuildResult() noexcept;

	FrameGraphBuildResult(const FrameGraphBuildResult&) = delete;
	FrameGraphBuildResult& operator=(const FrameGraphBuildResult&) = delete;
	FrameGraphBuildResult(FrameGraphBuildResult&&) noexcept;
	FrameGraphBuildResult& operator=(FrameGraphBuildResult&&) noexcept;

	std::unique_ptr<FrameGraph> Graph;
	FrameAssemblyResourceLayout Resources = {};
};

class FrameGraphFactory final
{
  public:
	explicit FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	static void ExportTextureIfValid(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle handle,
	    std::string_view name) noexcept;
	static void ExportFrameProductRoots(
	    FrameGraphBuilder& builder,
	    const FrameAssemblyResourceLayout& resources) noexcept;

	FrameGraphDependencies m_dependencies;
};
