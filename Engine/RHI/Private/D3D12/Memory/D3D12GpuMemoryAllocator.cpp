#include "PCH.h"

#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <D3D12MemAlloc.h>

static const auto g_d3d12MemoryLogger = Logging::GetOrCreateLogger("RHI.D3D12.Memory");

struct D3D12GpuMemoryAllocator::Impl
{
	D3D12MA::Allocator* allocator = nullptr;

	~Impl() noexcept
	{
		if (allocator != nullptr)
		{
			allocator->Release();
			allocator = nullptr;
		}
	}
};

D3D12GpuMemoryAllocator::D3D12GpuMemoryAllocator(IDXGIAdapter* adapter, ID3D12Device* device) noexcept :
    m_impl(std::make_unique<Impl>())
{
	if (adapter == nullptr || device == nullptr)
	{
		Diagnostics::Fail(g_d3d12MemoryLogger, __FILE__, __LINE__, "D3D12GpuMemoryAllocator requires a valid adapter and device");
	}

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pAdapter = adapter;
	allocatorDesc.pDevice = device;

	CHECK(D3D12MA::CreateAllocator(&allocatorDesc, &m_impl->allocator));

	if (Environment::GetFlag("SPARKLE_RHI_MEMORY_DIAGNOSTICS"))
	{
		SPDLOG_LOGGER_INFO(g_d3d12MemoryLogger, "D3D12MA allocator initialized");
	}
}

D3D12GpuMemoryAllocator::~D3D12GpuMemoryAllocator() noexcept = default;

bool D3D12GpuMemoryAllocator::IsInitialized() const noexcept
{
	return m_impl != nullptr && m_impl->allocator != nullptr;
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateTexture(
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* optimizedClearValue,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	return CreateResource(resourceDesc, initialState, optimizedClearValue, category, residencyClass, debugName);
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateBuffer(
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	return CreateResource(resourceDesc, initialState, nullptr, category, residencyClass, debugName);
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateResource(
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* optimizedClearValue,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->allocator == nullptr)
	{
		return {};
	}

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = ToHeapType(residencyClass);

	D3D12MA::Allocation* allocation = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	const HRESULT hr = m_impl->allocator->CreateResource(
	    &allocationDesc,
	    &resourceDesc,
	    initialState,
	    optimizedClearValue,
	    &allocation,
	    IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
	if (FAILED(hr) || allocation == nullptr || resource == nullptr)
	{
		if (allocation != nullptr)
		{
			allocation->Release();
		}
		return {};
	}

	auto record = std::make_unique<D3D12GpuAllocationRecord>();
	record->Resource = std::move(resource);
	record->Allocation = allocation;
	record->Category = category;
	record->ResidencyClass = residencyClass;
	record->DebugName = std::wstring(debugName);
	if (!record->DebugName.empty())
	{
		record->Resource->SetName(record->DebugName.c_str());
		record->Allocation->SetName(record->DebugName.c_str());
	}

	return record;
}

D3D12_HEAP_TYPE D3D12GpuMemoryAllocator::ToHeapType(RhiMemoryResidencyClass residencyClass) noexcept
{
	switch (residencyClass)
	{
		case RhiMemoryResidencyClass::HostUpload:
			return D3D12_HEAP_TYPE_UPLOAD;
		case RhiMemoryResidencyClass::HostReadback:
			return D3D12_HEAP_TYPE_READBACK;
		case RhiMemoryResidencyClass::DeviceLocal:
		case RhiMemoryResidencyClass::Transient:
		default:
			return D3D12_HEAP_TYPE_DEFAULT;
	}
}
