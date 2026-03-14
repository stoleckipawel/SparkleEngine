#include "PCH.h"
#include "D3D12Rhi.h"
#include "D3D12DebugLayer.h"
#include "Window.h"

D3D12Rhi::D3D12Rhi(bool requireDXRSupport) noexcept
{
#if ENGINE_GPU_VALIDATION
	m_debugLayer = std::make_unique<D3D12DebugLayer>();
#endif
	CreateFactory();
	CreateDevice(requireDXRSupport);

#if ENGINE_GPU_VALIDATION
	m_debugLayer->InitializeInfoQueue(m_device.Get());
#endif

	CheckShaderModel6Support();
	CreateCommandQueue();
	CreateCommandAllocators();
	CreateCommandLists();
	CreateFenceAndEvent();
}

void D3D12Rhi::SelectAdapter() noexcept
{
	const DXGI_GPU_PREFERENCE pref =
	    RHISettings::PreferHighPerformanceAdapter ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_MINIMUM_POWER;

	for (UINT i = 0;; ++i)
	{
		ComPtr<IDXGIAdapter1> candidate;
		HRESULT hr = m_dxgiFactory->EnumAdapterByGpuPreference(i, pref, IID_PPV_ARGS(candidate.ReleaseAndGetAddressOf()));
		if (hr != S_OK)
			break;

		DXGI_ADAPTER_DESC1 desc{};
		if (FAILED(candidate->GetDesc1(&desc)))
			continue;
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), m_desiredD3DFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			m_adapter = candidate;
			return;
		}
	}

	for (UINT i = 0;; ++i)
	{
		ComPtr<IDXGIAdapter1> candidate;
		HRESULT hr = m_dxgiFactory->EnumAdapters1(i, candidate.ReleaseAndGetAddressOf());
		if (hr != S_OK)
			break;

		DXGI_ADAPTER_DESC1 desc{};
		if (FAILED(candidate->GetDesc1(&desc)))
			continue;
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), m_desiredD3DFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			m_adapter = candidate;
			return;
		}
	}
}

void D3D12Rhi::CheckShaderModel6Support() const noexcept
{
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
	if (!m_device)
	{
		LOG_FATAL("CheckShaderModel6Support called before device creation");
	}

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
	{
		LOG_FATAL("Device does not support Shader Model 6.0. Minimum required for engine.");
	}
}

void D3D12Rhi::CreateFactory()
{
#if ENGINE_GPU_VALIDATION
	UINT dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT dxgiFactoryFlags = 0;
#endif
	CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));
}

void D3D12Rhi::CreateDevice(bool)
{
	SelectAdapter();
	if (!m_adapter)
	{
		LOG_FATAL("No suitable adapter found when creating device");
	}

	CHECK(D3D12CreateDevice(m_adapter.Get(), m_desiredD3DFeatureLevel, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())));
}

void D3D12Rhi::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	CHECK(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_cmdQueue.ReleaseAndGetAddressOf())));
}

void D3D12Rhi::CreateCommandAllocators()
{
	for (size_t i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator[i].ReleaseAndGetAddressOf())));
	}
}

void D3D12Rhi::CreateCommandLists()
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		CHECK(m_device->CreateCommandList(
		    0,
		    D3D12_COMMAND_LIST_TYPE_DIRECT,
		    m_cmdAllocator[i].Get(),
		    nullptr,
		    IID_PPV_ARGS(m_cmdList[i].ReleaseAndGetAddressOf())));

		CHECK(m_cmdList[i]->Close());
	}
}

void D3D12Rhi::CreateFenceAndEvent()
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		m_fenceValues[i] = 0;
	}

	CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!m_fenceEvent)
	{
		LOG_FATAL("Failed To Create Fence Event");
	}
}

void D3D12Rhi::CloseCommandList(uint32_t frameInFlightIndex) noexcept
{
	CHECK(m_cmdList[frameInFlightIndex]->Close());
}

void D3D12Rhi::ResetCommandAllocator(uint32_t frameInFlightIndex) noexcept
{
	if (!m_cmdAllocator[frameInFlightIndex])
	{
		LOG_FATAL("ResetCommandAllocator called with missing allocator");
		return;
	}

	CHECK(m_cmdAllocator[frameInFlightIndex]->Reset());
}

void D3D12Rhi::ResetCommandList(uint32_t frameInFlightIndex) noexcept
{
	if (!m_cmdList[frameInFlightIndex])
	{
		LOG_FATAL("ResetCommandList called without a valid command list");
		return;
	}
	if (!m_cmdAllocator[frameInFlightIndex])
	{
		LOG_FATAL("ResetCommandList called with missing allocator");
		return;
	}

	CHECK(m_cmdList[frameInFlightIndex]->Reset(m_cmdAllocator[frameInFlightIndex].Get(), nullptr));
}

void D3D12Rhi::ExecuteCommandList(uint32_t frameInFlightIndex) noexcept
{
	if (!m_cmdList[frameInFlightIndex] || !m_cmdQueue)
	{
		LOG_FATAL("ExecuteCommandList called without valid command list or queue");
	}

	ID3D12CommandList* ppcommandLists[] = {m_cmdList[frameInFlightIndex].Get()};
	m_cmdQueue->ExecuteCommandLists(1, ppcommandLists);
}

void D3D12Rhi::SetBarrier(
    uint32_t frameInFlightIndex,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter) noexcept
{
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;

	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;

	if (!m_cmdList[frameInFlightIndex])
	{
		LOG_FATAL("SetBarrier: command list is null");
	}

	m_cmdList[frameInFlightIndex]->ResourceBarrier(1, &barrier);
}

void D3D12Rhi::WaitForGPU(uint32_t frameInFlightIndex) noexcept
{
	const uint64_t fenceCurrentValue = m_fenceValues[frameInFlightIndex];
	if (!m_fence)
	{
		LOG_FATAL("WaitForGPU called without a fence");
	}

	const uint64_t fenceCompletedValue = m_fence->GetCompletedValue();
	if (fenceCompletedValue < fenceCurrentValue)
	{
		CHECK(m_fence->SetEventOnCompletion(fenceCurrentValue, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void D3D12Rhi::Signal(uint32_t frameInFlightIndex) noexcept
{
	const uint64_t currentFenceValue = m_nextFenceValue++;
	if (!m_cmdQueue || !m_fence)
	{
		LOG_FATAL("Signal called without command queue or fence");
	}

	CHECK(m_cmdQueue->Signal(m_fence.Get(), currentFenceValue));

	m_fenceValues[frameInFlightIndex] = currentFenceValue;
}

void D3D12Rhi::Flush() noexcept
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		Signal(i);
		WaitForGPU(i);
	}
}

D3D12Rhi::~D3D12Rhi() noexcept
{
	for (UINT i = 0; i < RHISettings::FramesInFlight; ++i)
	{
		m_cmdList[i].Reset();
		m_cmdAllocator[i].Reset();
		m_fenceValues[i] = 0;
	}

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

	m_fence.Reset();
	m_cmdQueue.Reset();

#if ENGINE_REPORT_LIVE_OBJECTS
	if (m_debugLayer)
	{
		m_debugLayer->ReportLiveDeviceObjects(m_device.Get());
	}
#endif

	m_device.Reset();
	m_adapter.Reset();
	m_dxgiFactory.Reset();

#if ENGINE_GPU_VALIDATION
	m_debugLayer.reset();
#endif
}