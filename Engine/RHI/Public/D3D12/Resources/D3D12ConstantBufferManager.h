// ============================================================================
// D3D12ConstantBufferManager.h
// Centralized constant buffer management with GPU/CPU synchronization.
// ----------------------------------------------------------------------------
// USAGE:
//   D3D12ConstantBufferManager cbManager(timer, rhi, window, ...);
//   auto gpuAddr = cbManager.GetPerFrameGpuAddress();
//   cbManager.UpdatePerFrame();
//
// DESIGN:
//   Per-Frame/Per-View CBs:
//     Use persistent ConstantBuffer<T> instances (one per frame-in-flight).
//     Updated once per frame, bound to root CBV slots.
//
//   Per-Object CBs:
//     Use FrameResourceManager's linear allocator for suballocation per-draw.
//     UpdatePerObjectXXX() returns a unique GPU VA per call.
//
// NOTES:
//   - Per-object allocations are thread-safe (atomic linear allocator)
//   - Per-frame/per-view updates should be called from main thread
// ============================================================================
#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include "RHIConfig.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12ConstantBuffer.h"

class Timer;
class Window;
class D3D12Rhi;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;
class UI;

class D3D12ConstantBufferManager final
{
  public:
	D3D12ConstantBufferManager(
	    Timer& timer,
	    D3D12Rhi& rhi,
	    Window& window,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12FrameResourceManager& frameResourceManager,
	    D3D12SwapChain& swapChain,
	    UI& ui);
	~D3D12ConstantBufferManager() noexcept;

	D3D12ConstantBufferManager(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager& operator=(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager(D3D12ConstantBufferManager&&) = delete;
	D3D12ConstantBufferManager& operator=(D3D12ConstantBufferManager&&) = delete;

	// -------------------------------------------------------------------------
	// GPU Address Accessors (for binding root CBVs)
	// -------------------------------------------------------------------------

	// Get GPU address of current frame's per-frame CB.
	D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameGpuAddress() const;

	// Get GPU address of current frame's per-view CB.
	D3D12_GPU_VIRTUAL_ADDRESS GetPerViewGpuAddress() const;

	// -------------------------------------------------------------------------
	// Update Methods
	// -------------------------------------------------------------------------

	// Update per-frame constant buffer. Call once per frame.
	void UpdatePerFrame();

	// Update per-view constant buffer. Call once per camera/view.
	void UpdatePerView(const PerViewConstantBufferData& data);

	// Update per-object VS constant buffer for a draw.
	// Any system can provide this data (Primitive, SkeletalMesh, etc.) without coupling.
	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectVS(const PerObjectVSConstantBufferData& data);

	// Update per-object PS constant buffer (material data).
	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectPS(const PerObjectPSConstantBufferData& data);

  private:
	// Per-Frame constant buffers (persistent, one per frame-in-flight)
	std::unique_ptr<D3D12ConstantBuffer<PerFrameConstantBufferData>> m_perFrameCB[RHISettings::FramesInFlight];

	// Per-View constant buffers (persistent, one per frame-in-flight)
	std::unique_ptr<D3D12ConstantBuffer<PerViewConstantBufferData>> m_perViewCB[RHISettings::FramesInFlight];

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	UI* m_ui = nullptr;
};
