#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include <cassert>
#include "D3D12LinearAllocator.h"
#include "RHIConfig.h"
#include "D3D12Rhi.h"

struct D3D12FrameResource
{
	D3D12LinearAllocator CbAllocator;
	uint64_t FenceValue = 0;
	uint32_t FrameIndex = 0;

	void Initialize(D3D12Rhi& rhi, uint64_t allocatorCapacity, uint32_t frameIdx)
	{
		FrameIndex = frameIdx;
		FenceValue = 0;

		wchar_t name[64];
		swprintf_s(name, L"FrameAllocator_%u", frameIdx);
		CbAllocator.Initialize(rhi, allocatorCapacity, name);
	}

	void Shutdown() { CbAllocator.Shutdown(); }

	void Reset() { CbAllocator.Reset(); }
};

class D3D12FrameResourceManager final
{
  public:
	static constexpr uint64_t DefaultCapacityPerFrame = 4 * 1024 * 1024;

	explicit D3D12FrameResourceManager(D3D12Rhi& rhi, uint64_t capacityPerFrame = DefaultCapacityPerFrame) :
	    m_capacityPerFrame(capacityPerFrame)
	{
		for (uint32_t i = 0; i < RHISettings::FramesInFlight; ++i)
		{
			m_frameResources[i].Initialize(rhi, capacityPerFrame, i);
		}
	}

	~D3D12FrameResourceManager()
	{
		for (auto& frame : m_frameResources)
		{
			frame.Shutdown();
		}
	}

	D3D12FrameResourceManager(const D3D12FrameResourceManager&) = delete;
	D3D12FrameResourceManager& operator=(const D3D12FrameResourceManager&) = delete;
	D3D12FrameResourceManager(D3D12FrameResourceManager&&) = delete;
	D3D12FrameResourceManager& operator=(D3D12FrameResourceManager&&) = delete;

	void BeginFrame(ID3D12Fence* fence, HANDLE fenceEvent, uint32_t frameIndex)
	{
		m_currentFrameIndex = frameIndex;
		D3D12FrameResource& frame = m_frameResources[frameIndex];

		const uint64_t completedFence = fence->GetCompletedValue();
		if (completedFence < frame.FenceValue)
		{
			HRESULT hr = fence->SetEventOnCompletion(frame.FenceValue, fenceEvent);
			if (SUCCEEDED(hr))
			{
				WaitForSingleObject(fenceEvent, INFINITE);
			}
		}

		frame.Reset();
	}

	void EndFrame(uint64_t fenceValue) { m_frameResources[m_currentFrameIndex].FenceValue = fenceValue; }

	D3D12LinearAllocator& GetCurrentAllocator() noexcept { return m_frameResources[m_currentFrameIndex].CbAllocator; }

	D3D12LinearAllocation Allocate(uint64_t size, uint64_t alignment = 256)
	{
		return GetCurrentAllocator().Allocate(size, alignment);
	}

	template <typename T> D3D12_GPU_VIRTUAL_ADDRESS AllocateConstantBuffer(const T& data)
	{
		return GetCurrentAllocator().AllocateAndCopy(data);
	}

	float GetCurrentUsagePercent() const noexcept
	{
		return m_frameResources[m_currentFrameIndex].CbAllocator.GetUsagePercent();
	}

	uint64_t GetMaxHighWaterMark() const noexcept
	{
		uint64_t maxHwm = 0;
		for (const auto& frame : m_frameResources)
		{
			maxHwm = (std::max) (maxHwm, frame.CbAllocator.GetHighWaterMark());
		}
		return maxHwm;
	}

	uint64_t GetCapacityPerFrame() const noexcept { return m_capacityPerFrame; }

  private:
	std::array<D3D12FrameResource, RHISettings::FramesInFlight> m_frameResources;
	uint64_t m_capacityPerFrame = DefaultCapacityPerFrame;
	uint32_t m_currentFrameIndex = 0;
};
