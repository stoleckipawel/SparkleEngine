#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <cstdint>
#include <memory>

#include "Frame/RhiFrameConstants.h"
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

class D3D12Rhi final
{
  public:
	explicit D3D12Rhi(RhiExternalFeatureHooks externalFeatureHooks = {}) noexcept;

	~D3D12Rhi() noexcept;

	D3D12Rhi(const D3D12Rhi&) = delete;
	D3D12Rhi& operator=(const D3D12Rhi&) = delete;
	D3D12Rhi(D3D12Rhi&&) = delete;
	D3D12Rhi& operator=(D3D12Rhi&&) = delete;

	void ResetCommandAllocator(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	void ResetCommandAllocator(uint32_t frameInFlightIndex) noexcept
	{
		ResetCommandAllocator(ERhiQueueType::Graphics, frameInFlightIndex);
	}

	void ResetCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	void ResetCommandList(uint32_t frameInFlightIndex) noexcept { ResetCommandList(ERhiQueueType::Graphics, frameInFlightIndex); }

	void CloseCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	void CloseCommandList(uint32_t frameInFlightIndex) noexcept { CloseCommandList(ERhiQueueType::Graphics, frameInFlightIndex); }

	void ExecuteCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	void ExecuteCommandList(uint32_t frameInFlightIndex) noexcept { ExecuteCommandList(ERhiQueueType::Graphics, frameInFlightIndex); }

	void SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept;
	uint32_t GetCurrentFrameIndex() const noexcept;

	RhiSubmissionToken Signal(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	RhiSubmissionToken Signal(uint32_t frameInFlightIndex) noexcept { return Signal(ERhiQueueType::Graphics, frameInFlightIndex); }

	void WaitForGPU(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept;
	void WaitForGPU(uint32_t frameInFlightIndex) noexcept { WaitForGPU(ERhiQueueType::Graphics, frameInFlightIndex); }
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept;
	void WaitForSubmission(RhiSubmissionToken token) noexcept;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept;
	std::uint64_t GetCompletedSubmissionValue(ERhiQueueType queueType) const noexcept;

	void Flush() noexcept;
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
	const ComPtr<ID3D12CommandAllocator>& GetCommandAllocator(uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12CommandAllocator>& GetCommandAllocator(ERhiQueueType queueType, uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12GraphicsCommandList7>& GetCommandList(uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12GraphicsCommandList7>& GetCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12Fence1>& GetFence() const noexcept;
	const ComPtr<ID3D12Fence1>& GetFence(ERhiQueueType queueType) const noexcept;
	HANDLE GetFenceEvent() const noexcept;
	HANDLE GetFenceEvent(ERhiQueueType queueType) const noexcept;
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

  private:
	struct QueueState final
	{
		ComPtr<ID3D12CommandQueue> CommandQueue;
		ComPtr<ID3D12Fence1> Fence;
		HANDLE FenceEvent = nullptr;
		std::uint64_t NextSubmissionValue = 1;
		std::uint64_t LastSubmittedValue = 0;
		std::array<std::uint64_t, RhiFrameConstants::FramesInFlight> FrameSubmissionValues{};
	};

	void SelectAdapter() noexcept;
	void CreateFactory();
	void CreateDevice();
	void CreateMemoryAllocator();
	void CheckRayTracingSupport() noexcept;
	void RefreshPartitionedTlasCommandListCapability() noexcept;
	void CreateCommandQueue();
	void CreateCommandAllocators();
	void CreateCommandLists();
	void CreateFenceAndEvent();
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
	std::array<QueueState, RhiQueueTypeCount> m_queues{};
	ComPtr<ID3D12CommandQueue> m_externalCommandQueue = nullptr;
	std::array<std::array<ComPtr<ID3D12CommandAllocator>, RhiFrameConstants::FramesInFlight>, RhiQueueTypeCount> m_cmdAllocators{};
	std::array<std::array<ComPtr<ID3D12GraphicsCommandList7>, RhiFrameConstants::FramesInFlight>, RhiQueueTypeCount> m_cmdLists{};
	uint32_t m_currentFrameIndex = 0;
	D3D_FEATURE_LEVEL m_desiredD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	RhiRayTracingCapabilities m_rayTracingCapabilities = {};
	RhiExternalFeatureHooks m_externalFeatureHooks = {};
	bool m_externalFeatureHooksActive = false;
};
