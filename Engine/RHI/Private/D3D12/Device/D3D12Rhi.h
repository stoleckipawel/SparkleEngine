#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include "Commands/RhiQueue.h"
#include "D3D12/RayTracing/D3D12NvapiRayTracingProvider.h"
#include "Device/RenderHardwareInterface.h"
#include "Interop/RhiExternalFeatureHooks.h"

using Microsoft::WRL::ComPtr;

struct RhiDiagnosticMessage;

#if ENGINE_GPU_VALIDATION
class D3D12DebugLayer;
#endif

class D3D12GpuMemoryAllocator;
class D3D12CommandQueue;

class D3D12Rhi final
{
  public:
	explicit D3D12Rhi(RhiExternalFeatureHooks externalFeatureHooks = {}) noexcept;

	~D3D12Rhi() noexcept;

	D3D12Rhi(const D3D12Rhi&) = delete;
	D3D12Rhi& operator=(const D3D12Rhi&) = delete;
	D3D12Rhi(D3D12Rhi&&) = delete;
	D3D12Rhi& operator=(D3D12Rhi&&) = delete;

	RhiSubmissionToken SubmitCommandLists(
	    ERhiQueueType queueType,
	    std::span<ID3D12CommandList* const> commandLists,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept;

	void SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept;
	uint32_t GetCurrentFrameIndex() const noexcept;

	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept;
	void WaitForSubmission(RhiSubmissionToken token) noexcept;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept;
	std::uint64_t GetCompletedSubmissionValue(ERhiQueueType queueType) const noexcept;

	void WaitForIdle() noexcept;
	bool IsValidationEnabled() const noexcept;
	bool SupportsDebugMessages() const noexcept;
	bool TryPopDebugMessage(RhiDiagnosticMessage& outMessage) noexcept;
	void ClearDebugMessages() noexcept;
	bool SupportsLiveObjectReports() const noexcept;
	bool SupportsCrashDiagnostics() const noexcept;
	void ReportLiveObjects() noexcept;
	void CollectCrashDiagnostics() noexcept;

	void CheckShaderModel6Support() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;

	const ComPtr<IDXGIFactory7>& GetDxgiFactory() const noexcept;
	const ComPtr<IDXGIAdapter1>& GetAdapter() const noexcept;
	const ComPtr<ID3D12Device10>& GetDevice() const noexcept;
	const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const noexcept;
	const ComPtr<ID3D12CommandQueue>& GetCommandQueue(ERhiQueueType queueType) const noexcept;
	ID3D12CommandQueue* GetPresentationCommandQueue() const noexcept;
	const ComPtr<ID3D12Fence1>& GetFence() const noexcept;
	HANDLE GetFenceEvent() const noexcept;
	D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const noexcept;
	D3D12NvapiRayTracingProvider& GetNvapiRayTracingProvider() noexcept;
	const D3D12NvapiRayTracingProvider& GetNvapiRayTracingProvider() const noexcept;
	D3D12GpuMemoryAllocator& GetMemoryAllocator() noexcept;
	const D3D12GpuMemoryAllocator& GetMemoryAllocator() const noexcept;
	bool TryUpgradeExternalInterface(
	    ERhiExternalInterfaceKind kind,
	    IUnknown* nativeInterface,
	    REFIID requestedInterface,
	    void** upgradedInterface) noexcept;
	bool TryResolveExternalNativeInterface(
	    ERhiExternalInterfaceKind kind,
	    IUnknown* externalInterface,
	    REFIID requestedInterface,
	    void** nativeInterface) noexcept;
	void NotifyExternalPresentationReady(bool ready) noexcept;
	void ShutdownExternalRuntime() noexcept;

  private:
	void SelectAdapter() noexcept;
	bool IsNvidiaAdapter() const noexcept;
	void CreateFactory();
	void CreateDevice();
	void CreateMemoryAllocator();
	void CheckRayTracingSupport() noexcept;
	void RefreshPartitionedTlasCommandListCapability() noexcept;
	void SelectRayTracingTopLevelProvider() noexcept;
	void CreateCommandQueues();
	void DisableExternalFeatureHooks() noexcept;

#if ENGINE_GPU_VALIDATION
	std::unique_ptr<D3D12DebugLayer> m_debugLayer;
#endif

	ComPtr<IDXGIFactory7> m_dxgiFactory = nullptr;
	ComPtr<IDXGIAdapter1> m_adapter = nullptr;
	ComPtr<ID3D12Device10> m_device = nullptr;
	ComPtr<ID3D12Device10> m_externalDevice = nullptr;
	D3D12NvapiRayTracingProvider m_nvapiRayTracingProvider;
	std::unique_ptr<D3D12GpuMemoryAllocator> m_memoryAllocator;
	std::array<std::unique_ptr<D3D12CommandQueue>, RhiQueueTypeCount> m_queues{};
	ComPtr<ID3D12CommandQueue> m_externalCommandQueue = nullptr;
	uint32_t m_currentFrameIndex = 0;
	D3D_FEATURE_LEVEL m_desiredD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	RhiRayTracingCapabilities m_rayTracingCapabilities = {};
	RhiExternalFeatureHooks m_externalFeatureHooks = {};
	bool m_externalFeatureHooksActive = false;
};
