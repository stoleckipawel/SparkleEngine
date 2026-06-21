#include "PCH.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Diagnostics/D3D12DebugLayer.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "CVars/RHICVars.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "RHI/Public/Diagnostics/RhiDiagnostics.h"

static const auto g_d3d12RhiLogger = Logging::GetOrCreateLogger("RHI.D3D12");
static constexpr std::uint32_t kD3D12RayTracingMaxDeclarableShaderPayloadSizeInBytes = 4096;
static constexpr std::uint32_t kNvidiaVendorId = 0x10DE;

static bool IsNvidiaAdapter(IDXGIAdapter1* adapter) noexcept
{
	if (adapter == nullptr)
	{
		return false;
	}

	DXGI_ADAPTER_DESC1 adapterDesc{};
	return SUCCEEDED(adapter->GetDesc1(&adapterDesc)) && adapterDesc.VendorId == kNvidiaVendorId;
}

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
		SPARKLE_CPU_SCOPE("RHI.D3D12.RefreshPartitionedTlasCommandListCapability");
		RefreshPartitionedTlasCommandListCapability();
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
	    CVarPreferHighPerformanceAdapter.Get() ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_MINIMUM_POWER;

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
	m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
	    .Supported = false,
	    .Provider = ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas,
	    .RequiresNvidiaDevice = true,
	    .RunsOnNvidiaDevice = IsNvidiaAdapter(m_adapter.Get()),
	    .CapabilityStatusReason = "d3d12-options5-not-queried"};
	m_rayTracingCapabilities.Groups.Provider = RhiRayTracingProviderCapabilities{
	    .SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None,
	    .SelectedTopLevelProviderReason = "ray-tracing-not-queried"};
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
		if (m_rayTracingCapabilities.SupportsRayTracing)
		{
			m_rayTracingCapabilities.MaxTraceRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
			m_rayTracingCapabilities.MaxRayPayloadSizeInBytes = kD3D12RayTracingMaxDeclarableShaderPayloadSizeInBytes;
			m_rayTracingCapabilities.MaxRayAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
			m_rayTracingCapabilities.ShaderGroupHandleSizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			m_rayTracingCapabilities.ShaderTableAlignmentInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			m_rayTracingCapabilities.ShaderTableRecordAlignmentInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
			m_rayTracingCapabilities.AccelerationStructureByteAlignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
			m_rayTracingCapabilities.ScratchBufferByteAlignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
			m_rayTracingCapabilities.InstanceDescSizeInBytes = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
		}

		m_rayTracingCapabilities.Groups.AccelerationStructures = RhiAccelerationStructureCapabilities{
		    .SupportsRayTracing = m_rayTracingCapabilities.SupportsRayTracing,
		    .SupportsInlineRayQuery = m_rayTracingCapabilities.SupportsInlineRayQuery,
		    .SupportsAccelerationStructureShaderBinding = m_rayTracingCapabilities.SupportsRayTracing,
		    .MaxTraceRecursionDepth = m_rayTracingCapabilities.MaxTraceRecursionDepth,
		    .MaxRayPayloadSizeInBytes = m_rayTracingCapabilities.MaxRayPayloadSizeInBytes,
		    .MaxRayAttributeSizeInBytes = m_rayTracingCapabilities.MaxRayAttributeSizeInBytes,
		    .ShaderGroupHandleSizeInBytes = m_rayTracingCapabilities.ShaderGroupHandleSizeInBytes,
		    .ShaderTableAlignmentInBytes = m_rayTracingCapabilities.ShaderTableAlignmentInBytes,
		    .ShaderTableRecordAlignmentInBytes = m_rayTracingCapabilities.ShaderTableRecordAlignmentInBytes,
		    .AccelerationStructureByteAlignment = m_rayTracingCapabilities.AccelerationStructureByteAlignment,
		    .ScratchBufferByteAlignment = m_rayTracingCapabilities.ScratchBufferByteAlignment};
		m_rayTracingCapabilities.Groups.ClassicTlas = RhiClassicTlasCapabilities{
		    .SupportsClassicTlasBuild = m_rayTracingCapabilities.SupportsRayTracing,
		    .SupportsClassicTlasUpdate = m_rayTracingCapabilities.SupportsRayTracing,
		    .SupportsGpuReadableInstanceBuffer = m_rayTracingCapabilities.SupportsRayTracing,
		    .InstanceDescSizeInBytes = m_rayTracingCapabilities.InstanceDescSizeInBytes};
		m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
		    .Supported = false,
		    .Provider = ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas,
		    .RequiresNvidiaDevice = true,
		    .RunsOnNvidiaDevice = IsNvidiaAdapter(m_adapter.Get()),
		    .SupportsD3D12DeviceInterface = m_device != nullptr,
		    .SupportsD3D12PublicDxrPartitionedTlas = false,
		    .SupportsD3D12PublicDxrHeaders = false,
		    .CapabilityStatusReason = "d3d12-nvapi-ptlas-provider-not-queried"};
		m_rayTracingCapabilities.Groups.PartitionedTlas = m_nvapiRayTracingProvider.QueryPartitionedTlasCapabilities(
		    m_device.Get(),
		    IsNvidiaAdapter(m_adapter.Get()),
		    m_rayTracingCapabilities.SupportsRayTracing);
		m_rayTracingCapabilities.Groups.PartitionedTlas.SupportsD3D12PublicDxrPartitionedTlas = false;
		m_rayTracingCapabilities.Groups.PartitionedTlas.SupportsD3D12PublicDxrHeaders = false;

		const bool partitionedTlasRequestedAndSupported =
		    CVarRayTracingPreferPartitionedTlas.Get() && m_rayTracingCapabilities.Groups.PartitionedTlas.Supported;
		m_rayTracingCapabilities.Groups.Provider = RhiRayTracingProviderCapabilities{
		    .SelectedTopLevelProvider =
		        m_rayTracingCapabilities.SupportsRayTracing ? ERhiRayTracingTopLevelProvider::ClassicTlas : ERhiRayTracingTopLevelProvider::None,
		    .SelectedTopLevelProviderReason =
		        partitionedTlasRequestedAndSupported
		            ? "d3d12-nvapi-ptlas-supported-but-renderer-selection-not-wired"
		            : (m_rayTracingCapabilities.SupportsRayTracing ? "classic-tlas-baseline-selected" : "ray-tracing-unavailable")};

		SPDLOG_LOGGER_INFO(
		    g_d3d12RhiLogger,
		    "DXR capability: tier={}({}) rayTracing={} inlineRayQuery={} topLevelProvider={} partitionedTlasSupported={}",
		    static_cast<int>(options5.RaytracingTier),
		    RaytracingTierToString(options5.RaytracingTier),
		    m_rayTracingCapabilities.SupportsRayTracing,
		    m_rayTracingCapabilities.SupportsInlineRayQuery,
		    RhiRayTracingTopLevelProviderToString(m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProvider),
		    m_rayTracingCapabilities.Groups.PartitionedTlas.Supported);
	}
	else
	{
		m_rayTracingCapabilities.Groups.PartitionedTlas.CapabilityStatusReason = "d3d12-options5-query-failed";
		m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None;
		m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProviderReason = "d3d12-options5-query-failed";
		SPDLOG_LOGGER_WARN(
		    g_d3d12RhiLogger,
		    "CheckFeatureSupport(OPTIONS5) failed hr={:#010x}; ray tracing assumed unsupported.",
		    static_cast<uint32_t>(hr));
	}
}

void D3D12Rhi::RefreshPartitionedTlasCommandListCapability() noexcept
{
	RhiPartitionedTlasCapabilities& partitionedTlas = m_rayTracingCapabilities.Groups.PartitionedTlas;
	if (partitionedTlas.Provider != ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas)
	{
		return;
	}

	const bool supportsCommandListInterface = m_cmdList[0] != nullptr;
	partitionedTlas.SupportsD3D12CommandListInterface = supportsCommandListInterface;
	if (!supportsCommandListInterface)
	{
		if (partitionedTlas.SupportsD3D12NvapiPartitionedTlas)
		{
			partitionedTlas.Supported = false;
			partitionedTlas.CapabilityStatusReason = "d3d12-command-list-interface-missing";
		}
		return;
	}

	const bool canUseNvapiPartitionedTlas =
	    partitionedTlas.SupportsD3D12NvapiPartitionedTlas &&
	    partitionedTlas.SupportsD3D12NvapiHeaders &&
	    partitionedTlas.SupportsD3D12NvapiRuntime &&
	    partitionedTlas.SupportsD3D12DeviceInterface;
	if (canUseNvapiPartitionedTlas)
	{
		partitionedTlas.Supported = true;
		partitionedTlas.CapabilityStatusReason = "d3d12-nvapi-ptlas-standard-supported";
	}

	const bool partitionedTlasRequestedAndSupported = CVarRayTracingPreferPartitionedTlas.Get() && partitionedTlas.Supported;
	if (m_rayTracingCapabilities.SupportsRayTracing)
	{
		m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::ClassicTlas;
		m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProviderReason =
		    partitionedTlasRequestedAndSupported ? "d3d12-nvapi-ptlas-supported-but-renderer-selection-not-wired"
		                                         : "classic-tlas-baseline-selected";
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
	for (size_t i = 0; i < RhiFrameConstants::FramesInFlight; ++i)
	{
		CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmdAllocator[i].ReleaseAndGetAddressOf())));
	}
}

void D3D12Rhi::CreateCommandLists()
{
	for (UINT i = 0; i < RhiFrameConstants::FramesInFlight; ++i)
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
	for (UINT i = 0; i < RhiFrameConstants::FramesInFlight; ++i)
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

void D3D12Rhi::SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept
{
	m_currentFrameIndex = frameInFlightIndex;
}

uint32_t D3D12Rhi::GetCurrentFrameIndex() const noexcept
{
	return m_currentFrameIndex;
}

RhiRayTracingCapabilities D3D12Rhi::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingCapabilities;
}

const ComPtr<IDXGIFactory7>& D3D12Rhi::GetDxgiFactory() const noexcept
{
	return m_dxgiFactory;
}

const ComPtr<IDXGIAdapter1>& D3D12Rhi::GetAdapter() const noexcept
{
	return m_adapter;
}

const ComPtr<ID3D12Device10>& D3D12Rhi::GetDevice() const noexcept
{
	return m_device;
}

const ComPtr<ID3D12CommandQueue>& D3D12Rhi::GetCommandQueue() const noexcept
{
	return m_cmdQueue;
}

const ComPtr<ID3D12CommandAllocator>& D3D12Rhi::GetCommandAllocator(uint32_t frameInFlightIndex) const noexcept
{
	return m_cmdAllocator[frameInFlightIndex];
}

const ComPtr<ID3D12GraphicsCommandList7>& D3D12Rhi::GetCommandList(uint32_t frameInFlightIndex) const noexcept
{
	return m_cmdList[frameInFlightIndex];
}

const ComPtr<ID3D12Fence1>& D3D12Rhi::GetFence() const noexcept
{
	return m_fence;
}

HANDLE D3D12Rhi::GetFenceEvent() const noexcept
{
	return m_fenceEvent;
}

uint64_t D3D12Rhi::GetNextFenceValue() const noexcept
{
	return m_nextFenceValue;
}

D3D_FEATURE_LEVEL D3D12Rhi::GetDeviceFeatureLevel() const noexcept
{
	return m_desiredD3DFeatureLevel;
}

D3D12NvapiRayTracingProvider& D3D12Rhi::GetNvapiRayTracingProvider() noexcept
{
	return m_nvapiRayTracingProvider;
}

const D3D12NvapiRayTracingProvider& D3D12Rhi::GetNvapiRayTracingProvider() const noexcept
{
	return m_nvapiRayTracingProvider;
}

D3D12GpuMemoryAllocator& D3D12Rhi::GetMemoryAllocator() noexcept
{
	return *m_memoryAllocator;
}

const D3D12GpuMemoryAllocator& D3D12Rhi::GetMemoryAllocator() const noexcept
{
	return *m_memoryAllocator;
}

void D3D12Rhi::CloseCommandList(uint32_t frameInFlightIndex) noexcept
{
	const HRESULT closeResult = m_cmdList[frameInFlightIndex]->Close();
	if (FAILED(closeResult))
	{
		RhiDiagnosticMessage message;
		while (TryPopDebugMessage(message))
		{
			SPDLOG_LOGGER_ERROR(g_d3d12RhiLogger, "D3D12 debug message before command-list close failure: {}", message.Text);
		}
	}
	CHECK(closeResult);
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
	for (UINT i = 0; i < RhiFrameConstants::FramesInFlight; ++i)
	{
		Signal(i);
		WaitForGPU(i);
	}
}

bool D3D12Rhi::IsValidationEnabled() const noexcept
{
#if ENGINE_GPU_VALIDATION
	return m_debugLayer != nullptr;
#else
	return false;
#endif
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
	for (UINT i = 0; i < RhiFrameConstants::FramesInFlight; ++i)
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
