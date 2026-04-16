#pragma once

#include <memory>

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;
class RenderHardwareInterface;
class Window;

struct FrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	Window& window;
	RenderViewportExtent sceneExtent;
	bool presentSceneToBackBuffer = true;
};

struct FrameGraphBuildResult
{
	std::unique_ptr<FrameGraph> Graph;
	TextureHandle SceneColor;
	TextureHandle SceneDepth;
};

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};