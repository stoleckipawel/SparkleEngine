#pragma once

#include "Frame/Core/Frame.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <memory>
#include <string_view>

class FrameGraph;
class FrameGraphBuilder;
class RenderPassRuntimeCache;
class RenderHardwareInterface;
class Window;

struct FrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	const RenderPassRuntimeCache& renderPassRuntimeCache;
	Window& window;
	RenderViewportExtent renderExtent;
	RenderViewportExtent outputExtent;
	EngineExposureMeteringMethod exposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	FramePresentationTarget presentationTarget = FramePresentationTarget::BackBuffer;
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
	static void ExportTextureIfValid(FrameGraphBuilder& builder, FrameGraphTextureHandle handle, std::string_view name) noexcept;
	static void ExportFrameProductRoots(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources) noexcept;

	FrameGraphDependencies m_dependencies;
};
