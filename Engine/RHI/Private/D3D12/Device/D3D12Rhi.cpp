#include "PCH.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Diagnostics/D3D12DebugLayer.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "CVars/RHICVars.h"
#include "Window/Window.h"

#include "RHI/Public/Diagnostics/RhiDiagnostics.h"

static const auto g_d3d12RhiLogger = Logging::GetOrCreateLogger("RHI.D3D12");
static constexpr std::uint32_t kD3D12RayTracingMaxDeclarableShaderPayloadSizeInBytes = 4096;
static constexpr std::uint32_t kNvidiaVendorId = 0x10DE;

static D3D12_COMMAND_LIST_TYPE ToD3D12CommandListType(ERhiQueueType queueType) noexcept
{
	switch (queueType)
	{
		case ERhiQueueType::Graphics:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case ERhiQueueType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case ERhiQueueType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		case ERhiQueueType::Count:
			break;
	}

	Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Invalid RHI queue type");
	return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

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

D3D12Rhi::D3D12Rhi(RhiExternalFeatureHooks externalFeatureHooks) noexcept :
    m_externalFeatureHooks(externalFeatureHooks)
{
#if ENGINE_GPU_VALIDATION
	m_debugLayer = std::make_unique<D3D12DebugLayer>();
#endif
	{
		CreateFactory();
	}
	{
		CreateDevice();
	}

#if ENGINE_GPU_VALIDATION
	m_debugLayer->InitializeInfoQueue(m_device.Get());
#endif

	{
		CreateMemoryAllocator();
	}

	{
		CheckShaderModel6Support();
	}
	{
		CreateCommandQueue();
	}
	{
		CreateCommandAllocators();
	}
	{
		CreateCommandLists();
	}
	{
		RefreshPartitionedTlasCommandListCapability();
	}
	{
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
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	if (!m_device)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "CheckShaderModel6Support called before device creation");
	}

	HRESULT hr = m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
	if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Device does not support Shader Model 6.6. Minimum required for engine.");
	}
}

void D3D12Rhi::CreateFactory()
{
#if ENGINE_GPU_VALIDATION
	UINT dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT dxgiFactoryFlags = 0;
#endif
	ComPtr<IDXGIFactory7> createdFactory;
	CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(createdFactory.ReleaseAndGetAddressOf())));
	if (m_externalFeatureHooks.ResolveNativeInterface != nullptr)
	{
		ComPtr<IDXGIFactory7> nativeFactory;
		if (TryResolveExternalNativeInterface(
		        ERhiExternalInterfaceKind::PresentationFactory,
		        createdFactory.Get(),
		        IID_PPV_ARGS(nativeFactory.ReleaseAndGetAddressOf())))
		{
			m_dxgiFactory = std::move(nativeFactory);
			return;
		}
	}
	m_dxgiFactory = std::move(createdFactory);
}

void D3D12Rhi::CreateDevice()
{
	SelectAdapter();
	if (!m_adapter)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "No suitable adapter found when creating device");
	}

	ComPtr<ID3D12Device10> createdDevice;
	CHECK(D3D12CreateDevice(m_adapter.Get(), m_desiredD3DFeatureLevel, IID_PPV_ARGS(createdDevice.ReleaseAndGetAddressOf())));
	if (m_externalFeatureHooks.ResolveNativeInterface != nullptr)
	{
		ComPtr<ID3D12Device10> nativeDevice;
		if (TryResolveExternalNativeInterface(
		        ERhiExternalInterfaceKind::GraphicsDevice,
		        createdDevice.Get(),
		        IID_PPV_ARGS(nativeDevice.ReleaseAndGetAddressOf())))
		{
			m_device = std::move(nativeDevice);
		}
	}
	if (m_device == nullptr)
	{
		m_device = std::move(createdDevice);
	}
	if (m_externalFeatureHooks.DeviceCreated != nullptr)
	{
		m_externalFeatureHooksActive = m_externalFeatureHooks.DeviceCreated(
		    ERhiBackendApi::D3D12,
		    NativeGraphicsDeviceHandle{m_device.Get()},
		    m_externalFeatureHooks.UserData);
	}
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

	const bool supportsCommandListInterface = GetCommandList(ERhiQueueType::Graphics, 0) != nullptr;
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
	D3D12_COMMAND_QUEUE_DESC graphicsQueueDesc = {};
	graphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	graphicsQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	graphicsQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	graphicsQueueDesc.NodeMask = 0;
	if (m_externalFeatureHooksActive)
	{
		ComPtr<ID3D12Device10> externalDevice;
		if (TryUpgradeExternalInterface(
		        ERhiExternalInterfaceKind::GraphicsDevice,
		        m_device.Get(),
		        IID_PPV_ARGS(externalDevice.ReleaseAndGetAddressOf())))
		{
			ComPtr<ID3D12CommandQueue> externalQueue;
			const HRESULT createResult = externalDevice->CreateCommandQueue(
			    &graphicsQueueDesc,
			    IID_PPV_ARGS(externalQueue.ReleaseAndGetAddressOf()));
			if (SUCCEEDED(createResult))
			{
				ComPtr<ID3D12CommandQueue> nativeQueue;
				if (TryResolveExternalNativeInterface(
				        ERhiExternalInterfaceKind::GraphicsQueue,
				        externalQueue.Get(),
				        IID_PPV_ARGS(nativeQueue.ReleaseAndGetAddressOf())))
				{
					m_externalDevice = std::move(externalDevice);
					m_externalCommandQueue = std::move(externalQueue);
					m_queues[RhiQueueTypeToIndex(ERhiQueueType::Graphics)].CommandQueue = std::move(nativeQueue);
				}
			}
		}

		if (m_queues[RhiQueueTypeToIndex(ERhiQueueType::Graphics)].CommandQueue == nullptr)
		{
			DisableExternalFeatureHooks();
		}
	}

	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueState& queue = m_queues[queueIndex];
		if (queue.CommandQueue != nullptr)
		{
			continue;
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc = graphicsQueueDesc;
		queueDesc.Type = ToD3D12CommandListType(static_cast<ERhiQueueType>(queueIndex));
		CHECK(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queue.CommandQueue.ReleaseAndGetAddressOf())));
	}
}

void D3D12Rhi::CreateCommandAllocators()
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const D3D12_COMMAND_LIST_TYPE commandListType = ToD3D12CommandListType(static_cast<ERhiQueueType>(queueIndex));
		for (std::size_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
		{
			CHECK(m_device->CreateCommandAllocator(
			    commandListType,
			    IID_PPV_ARGS(m_cmdAllocators[queueIndex][frameIndex].ReleaseAndGetAddressOf())));
		}
	}
}

void D3D12Rhi::CreateCommandLists()
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const D3D12_COMMAND_LIST_TYPE commandListType = ToD3D12CommandListType(static_cast<ERhiQueueType>(queueIndex));
		for (std::size_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
		{
			CHECK(m_device->CreateCommandList(
			    0,
			    commandListType,
			    m_cmdAllocators[queueIndex][frameIndex].Get(),
			    nullptr,
			    IID_PPV_ARGS(m_cmdLists[queueIndex][frameIndex].ReleaseAndGetAddressOf())));

			CHECK(m_cmdLists[queueIndex][frameIndex]->Close());
		}
	}
}

void D3D12Rhi::CreateFenceAndEvent()
{
	for (QueueState& queue : m_queues)
	{
		CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(queue.Fence.ReleaseAndGetAddressOf())));
		queue.FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		if (queue.FenceEvent == nullptr)
		{
			Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Failed to create queue fence event");
		}
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
	return GetCommandQueue(ERhiQueueType::Graphics);
}

const ComPtr<ID3D12CommandQueue>& D3D12Rhi::GetCommandQueue(ERhiQueueType queueType) const noexcept
{
	return m_queues[RhiQueueTypeToIndex(queueType)].CommandQueue;
}

ID3D12CommandQueue* D3D12Rhi::GetPresentationCommandQueue() const noexcept
{
	return m_externalCommandQueue != nullptr ? m_externalCommandQueue.Get() : GetCommandQueue().Get();
}

const ComPtr<ID3D12CommandAllocator>& D3D12Rhi::GetCommandAllocator(uint32_t frameInFlightIndex) const noexcept
{
	return GetCommandAllocator(ERhiQueueType::Graphics, frameInFlightIndex);
}

const ComPtr<ID3D12CommandAllocator>& D3D12Rhi::GetCommandAllocator(
	ERhiQueueType queueType,
	uint32_t frameInFlightIndex) const noexcept
{
	return m_cmdAllocators[RhiQueueTypeToIndex(queueType)][frameInFlightIndex];
}

const ComPtr<ID3D12GraphicsCommandList7>& D3D12Rhi::GetCommandList(uint32_t frameInFlightIndex) const noexcept
{
	return GetCommandList(ERhiQueueType::Graphics, frameInFlightIndex);
}

const ComPtr<ID3D12GraphicsCommandList7>& D3D12Rhi::GetCommandList(
	ERhiQueueType queueType,
	uint32_t frameInFlightIndex) const noexcept
{
	return m_cmdLists[RhiQueueTypeToIndex(queueType)][frameInFlightIndex];
}

const ComPtr<ID3D12Fence1>& D3D12Rhi::GetFence() const noexcept
{
	return GetFence(ERhiQueueType::Graphics);
}

const ComPtr<ID3D12Fence1>& D3D12Rhi::GetFence(ERhiQueueType queueType) const noexcept
{
	return m_queues[RhiQueueTypeToIndex(queueType)].Fence;
}

HANDLE D3D12Rhi::GetFenceEvent() const noexcept
{
	return GetFenceEvent(ERhiQueueType::Graphics);
}

HANDLE D3D12Rhi::GetFenceEvent(ERhiQueueType queueType) const noexcept
{
	return m_queues[RhiQueueTypeToIndex(queueType)].FenceEvent;
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

bool D3D12Rhi::TryUpgradeExternalInterface(
    ERhiExternalInterfaceKind kind,
    IUnknown* nativeInterface,
    REFIID requestedInterface,
    void** upgradedInterface) noexcept
{
	if (!m_externalFeatureHooksActive || m_externalFeatureHooks.UpgradeInterface == nullptr ||
	    nativeInterface == nullptr || upgradedInterface == nullptr)
	{
		return false;
	}

	*upgradedInterface = nullptr;
	void* candidate = nativeInterface;
	if (!m_externalFeatureHooks.UpgradeInterface(
	        ERhiBackendApi::D3D12,
	        kind,
	        &candidate,
	        m_externalFeatureHooks.UserData) ||
	    candidate == nullptr)
	{
		return false;
	}

	IUnknown* candidateInterface = static_cast<IUnknown*>(candidate);
	const HRESULT result = candidateInterface->QueryInterface(requestedInterface, upgradedInterface);
	if (candidateInterface != nativeInterface)
	{
		// Interface upgrades transfer the initial proxy reference. QueryInterface
		// above produced the reference retained by the RHI.
		candidateInterface->Release();
	}
	return SUCCEEDED(result) && *upgradedInterface != nullptr;
}

bool D3D12Rhi::TryResolveExternalNativeInterface(
    ERhiExternalInterfaceKind kind,
    IUnknown* externalInterface,
    REFIID requestedInterface,
    void** nativeInterface) noexcept
{
	if (m_externalFeatureHooks.ResolveNativeInterface == nullptr || externalInterface == nullptr || nativeInterface == nullptr)
	{
		return false;
	}

	*nativeInterface = nullptr;
	void* resolved = nullptr;
	if (!m_externalFeatureHooks.ResolveNativeInterface(
	        ERhiBackendApi::D3D12,
	        kind,
	        externalInterface,
	        &resolved,
	        m_externalFeatureHooks.UserData) ||
	    resolved == nullptr)
	{
		return false;
	}

	IUnknown* resolvedInterface = static_cast<IUnknown*>(resolved);
	const HRESULT result = resolvedInterface->QueryInterface(requestedInterface, nativeInterface);
	// Native-interface resolution transfers one reference regardless of whether
	// the returned pointer aliases the input.
	resolvedInterface->Release();
	return SUCCEEDED(result) && *nativeInterface != nullptr;
}

void D3D12Rhi::NotifyExternalPresentationReady(bool ready) noexcept
{
	if (m_externalFeatureHooks.PresentationReady != nullptr)
	{
		m_externalFeatureHooks.PresentationReady(
		    ERhiBackendApi::D3D12,
		    ready && m_externalFeatureHooksActive,
		    m_externalFeatureHooks.UserData);
	}
}

void D3D12Rhi::DisableExternalFeatureHooks() noexcept
{
	m_externalFeatureHooksActive = false;
	NotifyExternalPresentationReady(false);
}

void D3D12Rhi::CloseCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	const ComPtr<ID3D12GraphicsCommandList7>& commandList = GetCommandList(queueType, frameInFlightIndex);
	if (commandList == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "CloseCommandList called without a valid command list");
		return;
	}

	const HRESULT closeResult = commandList->Close();
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

void D3D12Rhi::ResetCommandAllocator(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	const ComPtr<ID3D12CommandAllocator>& allocator = GetCommandAllocator(queueType, frameInFlightIndex);
	if (allocator == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandAllocator called with missing allocator");
		return;
	}

	CHECK(allocator->Reset());
}

void D3D12Rhi::ResetCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	const ComPtr<ID3D12GraphicsCommandList7>& commandList = GetCommandList(queueType, frameInFlightIndex);
	const ComPtr<ID3D12CommandAllocator>& allocator = GetCommandAllocator(queueType, frameInFlightIndex);
	if (commandList == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandList called without a valid command list");
		return;
	}
	if (allocator == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ResetCommandList called with missing allocator");
		return;
	}

	CHECK(commandList->Reset(allocator.Get(), nullptr));
}

void D3D12Rhi::ExecuteCommandList(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	const ComPtr<ID3D12GraphicsCommandList7>& commandList = GetCommandList(queueType, frameInFlightIndex);
	const ComPtr<ID3D12CommandQueue>& commandQueue = GetCommandQueue(queueType);
	if (commandList == nullptr || commandQueue == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "ExecuteCommandList called without valid command list or queue");
		return;
	}

	ID3D12CommandList* commandLists[] = {commandList.Get()};
	commandQueue->ExecuteCommandLists(1, commandLists);
}

void D3D12Rhi::WaitForGPU(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	const QueueState& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	const RhiSubmissionToken token{.Queue = queueType, .Value = queue.FrameSubmissionValues[frameInFlightIndex]};
	WaitForSubmission(token);
}

RhiSubmissionToken D3D12Rhi::Signal(ERhiQueueType queueType, uint32_t frameInFlightIndex) noexcept
{
	QueueState& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	if (queue.CommandQueue == nullptr || queue.Fence == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "Signal called without command queue or fence");
		return {};
	}

	const std::uint64_t submissionValue = queue.NextSubmissionValue++;
	CHECK(queue.CommandQueue->Signal(queue.Fence.Get(), submissionValue));
	queue.LastSubmittedValue = submissionValue;
	queue.FrameSubmissionValues[frameInFlightIndex] = submissionValue;
	return RhiSubmissionToken{.Queue = queueType, .Value = submissionValue};
}

void D3D12Rhi::QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept
{
	if (!executionToken.IsValid() || waitQueue == executionToken.Queue)
	{
		return;
	}

	QueueState& waitingQueue = m_queues[RhiQueueTypeToIndex(waitQueue)];
	const QueueState& executionQueue = m_queues[RhiQueueTypeToIndex(executionToken.Queue)];
	if (waitingQueue.CommandQueue == nullptr || executionQueue.Fence == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "QueueWait called without valid queue synchronization state");
		return;
	}

	CHECK(waitingQueue.CommandQueue->Wait(executionQueue.Fence.Get(), executionToken.Value));
}

void D3D12Rhi::WaitForSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		return;
	}

	const QueueState& queue = m_queues[RhiQueueTypeToIndex(token.Queue)];
	if (queue.Fence == nullptr || queue.FenceEvent == nullptr)
	{
		Diagnostics::Fail(g_d3d12RhiLogger, __FILE__, __LINE__, "WaitForSubmission called without a fence or event");
		return;
	}

	if (queue.Fence->GetCompletedValue() < token.Value)
	{
		CHECK(queue.Fence->SetEventOnCompletion(token.Value, queue.FenceEvent));
		WaitForSingleObject(queue.FenceEvent, INFINITE);
	}
}

bool D3D12Rhi::IsSubmissionComplete(RhiSubmissionToken token) const noexcept
{
	if (!token.IsValid())
	{
		return true;
	}

	const QueueState& queue = m_queues[RhiQueueTypeToIndex(token.Queue)];
	return queue.Fence != nullptr && queue.Fence->GetCompletedValue() >= token.Value;
}

RhiSubmissionToken D3D12Rhi::GetLastSubmittedToken(ERhiQueueType queueType) const noexcept
{
	return RhiSubmissionToken{
	    .Queue = queueType,
	    .Value = m_queues[RhiQueueTypeToIndex(queueType)].LastSubmittedValue};
}

std::uint64_t D3D12Rhi::GetCompletedSubmissionValue(ERhiQueueType queueType) const noexcept
{
	const QueueState& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	return queue.Fence != nullptr ? queue.Fence->GetCompletedValue() : 0;
}

void D3D12Rhi::Flush() noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
		WaitForSubmission(Signal(queueType, m_currentFrameIndex));
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
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		for (std::size_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
		{
			m_cmdLists[queueIndex][frameIndex].Reset();
			m_cmdAllocators[queueIndex][frameIndex].Reset();
		}

		QueueState& queue = m_queues[queueIndex];
		if (queue.FenceEvent != nullptr)
		{
			CloseHandle(queue.FenceEvent);
			queue.FenceEvent = nullptr;
		}
		queue.Fence.Reset();
		queue.CommandQueue.Reset();
	}

	m_externalCommandQueue.Reset();
	m_externalDevice.Reset();
	m_memoryAllocator.reset();

#if ENGINE_GPU_VALIDATION
	m_debugLayer.reset();
#endif

	m_device.Reset();
	m_adapter.Reset();
	m_dxgiFactory.Reset();
}
