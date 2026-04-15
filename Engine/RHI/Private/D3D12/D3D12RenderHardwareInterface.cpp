#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain) noexcept :
    m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain)
{
}

RhiBackendApi D3D12RenderHardwareInterface::GetBackendApi() const noexcept
{
	return RhiBackendApi::D3D12;
}

std::uint32_t D3D12RenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetCurrentFrameIndex() : 0u;
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr};
}

NativeGraphicsCommandListHandle D3D12RenderHardwareInterface::GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept
{
	return NativeGraphicsCommandListHandle{m_rhi != nullptr ? m_rhi->GetCommandList(frameIndex).Get() : nullptr};
}

NativeDescriptorHeapHandle D3D12RenderHardwareInterface::GetShaderResourceHeapHandle() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return NativeDescriptorHeapHandle{heap != nullptr ? heap->GetRaw() : nullptr};
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4])
    const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = static_cast<ID3D12GraphicsCommandList*>(commandList.Value);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToRenderTarget{};
	transitionToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToRenderTarget.Transition.pResource = presentTexture;
	transitionToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	transitionToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	nativeCommandList->ResourceBarrier(1, &transitionToRenderTarget);

	ID3D12DescriptorHeap* heaps[2] = {};
	UINT heapCount = 0;
	if (m_descriptorHeapManager != nullptr)
	{
		if (D3D12DescriptorHeap* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		{
			heaps[heapCount++] = srvHeap->GetRaw();
		}

		if (D3D12DescriptorHeap* samplerHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER))
		{
			heaps[heapCount++] = samplerHeap->GetRaw();
		}
	}

	if (heapCount > 0)
	{
		nativeCommandList->SetDescriptorHeaps(heapCount, heaps);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_swapChain->GetCPUHandle();
	nativeCommandList->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	nativeCommandList->ClearRenderTargetView(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor, 0, nullptr);
}

void D3D12RenderHardwareInterface::EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = static_cast<ID3D12GraphicsCommandList*>(commandList.Value);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToPresent{};
	transitionToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToPresent.Transition.pResource = presentTexture;
	transitionToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	transitionToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	nativeCommandList->ResourceBarrier(1, &transitionToPresent);
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}