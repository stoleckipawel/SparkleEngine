#include "PCH.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12DebugLayer.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "CVars/RHICVars.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Trace.h"

static const auto g_d3d12RhiLogger = Logging::GetOrCreateLogger("RHI.D3D12");

static const char* RaytracingTierToString(D3D12_RAYTRACING_TIER tier) noexcept
{
	switch (tier)
	{
		case D3D12_RAYTRACING_TIER_NOT_SUPPORTED:
			return "NotSupported";
		case D3D12_RAYTRACING_TIER_1_0:
			return "Tier1_0";
		case D3D12_RAYTRACING_TIER_1_1:
			return "Tier1_1";
		default:
			return "Unknown";
	}
}

D3D12Rhi::D3D12Rhi() noexcept
{
#if ENGINE_GPU_VALIDATION
	m_debugLayer = std::make_unique<D3D12DebugLayer>();
#endif
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateFactory");
		CreateFactory();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateDevice");
		CreateDevice();
	}

#if ENGINE_GPU_VALIDATION
	m_debugLayer->InitializeInfoQueue(m_device.Get());
#endif

	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateMemoryAllocator");
		CreateMemoryAllocator();
	}

	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CheckShaderModel");
		CheckShaderModel6Support();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateCommandQueue");
		CreateCommandQueue();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateCommandAllocators");
		CreateCommandAllocators();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateCommandLists");
		CreateCommandLists();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.D3D12.CreateFence");
		CreateFenceAndEvent();
	}

	constexpr uint32_t kInitialFrameIndex = 0;
	SetCurrentFrameIndex(kInitialFrameIndex);
	ResetCommandAllocator(kInitialFrameIndex);
	ResetCommandList(kInitialFrameIndex);
}

void D3D12Rhi::SelectAdapter() noexcept
{
	const DXGI_GPU_PREFERENCE pref =
	    CVarRhiPreferHighPerformanceAdapter.Get() ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_MINIMUM_POWER;

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
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "CheckShaderModel6Support called before device creation");
	}

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Device does not support Shader Model 6.0. Minimum required for engine.");
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

void D3D12Rhi::CreateDevice()
{
	SelectAdapter();
	if (!m_adapter)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "No suitable adapter found when creating device");
	}

	CHECK(D3D12CreateDevice(m_adapter.Get(), m_desiredD3DFeatureLevel, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())));
	CheckRayTracingSupport();
}

void D3D12Rhi::CreateMemoryAllocator()
{
	if (!m_adapter || !m_device)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "CreateMemoryAllocator called before adapter and device creation");
	}

	m_memoryAllocator = std::make_unique<D3D12GpuMemoryAllocator>(m_adapter.Get(), m_device.Get());
}

void D3D12Rhi::CheckRayTracingSupport() noexcept
{
	m_rayTracingCapabilities = {};
	if (!m_device)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "CheckRayTracingSupport called before device creation");
		return;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	const HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if (SUCCEEDED(hr))
	{
		m_rayTracingCapabilities.SupportsRayTracing = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
		m_rayTracingCapabilities.SupportsInlineRayQuery = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;

		SPDLOG_LOGGER_INFO(
		    g_d3d12RhiLogger,
		    "DXR capability: tier={}({}), SupportsPipelineRayTracing={}, SupportsInlineRayQuery={}",
		    static_cast<int>(options5.RaytracingTier),
		    RaytracingTierToString(options5.RaytracingTier),
		    m_rayTracingCapabilities.SupportsRayTracing,
		    m_rayTracingCapabilities.SupportsInlineRayQuery);
	}
	else
	{
		SPDLOG_LOGGER_WARN(
		    g_d3d12RhiLogger,
		    "CheckFeatureSupport(OPTIONS5) failed hr={:#010x}; ray tracing assumed unsupported.",
		    static_cast<uint32_t>(hr));
	}
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
	for (size_t i = 0; i < RenderConfig::FramesInFlight; ++i)
	{
		CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator[i].ReleaseAndGetAddressOf())));
	}
}

void D3D12Rhi::CreateCommandLists()
{
	for (UINT i = 0; i < RenderConfig::FramesInFlight; ++i)
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
	for (UINT i = 0; i < RenderConfig::FramesInFlight; ++i)
	{
		m_fenceValues[i] = 0;
	}

	CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!m_fenceEvent)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Failed To Create Fence Event");
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
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandAllocator called with missing allocator");
		return;
	}

	CHECK(m_cmdAllocator[frameInFlightIndex]->Reset());
}

void D3D12Rhi::ResetCommandList(uint32_t frameInFlightIndex) noexcept
{
	if (!m_cmdList[frameInFlightIndex])
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandList called without a valid command list");
		return;
	}
	if (!m_cmdAllocator[frameInFlightIndex])
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandList called with missing allocator");
		return;
	}

	CHECK(m_cmdList[frameInFlightIndex]->Reset(m_cmdAllocator[frameInFlightIndex].Get(), nullptr));
}

void D3D12Rhi::ExecuteCommandList(uint32_t frameInFlightIndex) noexcept
{
	if (!m_cmdList[frameInFlightIndex] || !m_cmdQueue)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ExecuteCommandList called without valid command list or queue");
	}

	ID3D12CommandList* ppcommandLists[] = {m_cmdList[frameInFlightIndex].Get()};
	m_cmdQueue->ExecuteCommandLists(1, ppcommandLists);
}

void D3D12Rhi::WaitForGPU(uint32_t frameInFlightIndex) noexcept
{
	const uint64_t fenceCurrentValue = m_fenceValues[frameInFlightIndex];
	if (!m_fence)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "WaitForGPU called without a fence");
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
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Signal called without command queue or fence");
	}

	CHECK(m_cmdQueue->Signal(m_fence.Get(), currentFenceValue));

	m_fenceValues[frameInFlightIndex] = currentFenceValue;
}

void D3D12Rhi::Flush() noexcept
{
	for (UINT i = 0; i < RenderConfig::FramesInFlight; ++i)
	{
		Signal(i);
		WaitForGPU(i);
	}
}

bool D3D12Rhi::SupportsDebugMessages() const noexcept
{
#if ENGINE_GPU_VALIDATION
	return m_debugLayer != nullptr && m_debugLayer->SupportsDebugMessages();
#else
	return false;
#endif
}

bool D3D12Rhi::TryPopDebugMessage(RhiDiagnosticMessage& outMessage) noexcept
{
#if ENGINE_GPU_VALIDATION
	return m_debugLayer != nullptr && m_debugLayer->TryPopMessage(outMessage);
#else
	static_cast<void>(outMessage);
	return false;
#endif
}

void D3D12Rhi::ClearDebugMessages() noexcept
{
#if ENGINE_GPU_VALIDATION
	if (m_debugLayer != nullptr)
	{
		m_debugLayer->ClearMessages();
	}
#endif
}

bool D3D12Rhi::SupportsLiveObjectReports() const noexcept
{
#if ENGINE_GPU_VALIDATION
	return m_debugLayer != nullptr && m_debugLayer->SupportsLiveObjectReports();
#else
	return false;
#endif
}

bool D3D12Rhi::SupportsCrashDiagnostics() const noexcept
{
#if ENGINE_GPU_VALIDATION
	return m_debugLayer != nullptr && m_debugLayer->SupportsCrashDiagnostics();
#else
	return false;
#endif
}

void D3D12Rhi::ReportLiveObjects() noexcept
{
#if ENGINE_GPU_VALIDATION
	if (m_debugLayer != nullptr)
	{
		m_debugLayer->ReportLiveObjects(m_device.Get());
	}
#endif
}

void D3D12Rhi::CollectCrashDiagnostics() noexcept
{
#if ENGINE_GPU_VALIDATION
	if (m_debugLayer != nullptr)
	{
		m_debugLayer->CollectCrashDiagnostics(m_device.Get());
	}
#endif
}

D3D12Rhi::~D3D12Rhi() noexcept
{
	for (UINT i = 0; i < RenderConfig::FramesInFlight; ++i)
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
	m_memoryAllocator.reset();

#if ENGINE_GPU_VALIDATION
	m_debugLayer.reset();
#endif

	m_device.Reset();
	m_adapter.Reset();
	m_dxgiFactory.Reset();
}
