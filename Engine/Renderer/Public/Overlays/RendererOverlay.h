#pragma once

#include <d3d12.h>

#include <functional>
#include <memory>

class Timer;
class LevelManager;
class GameScene;
class Window;
class D3D12Rhi;
class D3D12DescriptorHeapManager;
class D3D12SwapChain;

class IRendererOverlay
{
  public:
	virtual ~IRendererOverlay() noexcept = default;

	virtual void Update() = 0;
	virtual void Render(ID3D12GraphicsCommandList* commandList) noexcept = 0;
};

struct RendererOverlayContext
{
	Timer& timer;
	LevelManager& levelManager;
	GameScene& gameScene;
	D3D12Rhi& rhi;
	Window& window;
	D3D12DescriptorHeapManager& descriptorHeapManager;
	D3D12SwapChain& swapChain;
};

using RendererOverlayFactory = std::function<std::unique_ptr<IRendererOverlay>(RendererOverlayContext& context)>;