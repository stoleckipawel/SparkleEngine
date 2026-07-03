#include "PCH.h"

#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include "Core/Public/Formatting/HexFormat.h"

#include <D3D12MemAlloc.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <Windows.h>

static const auto g_d3d12MemoryLogger = Logging::GetOrCreateLogger("RHI.D3D12.Memory");

struct D3D12GpuMemoryAllocator::Impl
{
	D3D12MA::Allocator* allocator = nullptr;
	mutable std::mutex recordsMutex;
	std::vector<D3D12GpuAllocationRecord*> liveRecords;
	std::vector<D3D12GpuHeapRecord*> liveHeapRecords;

	~Impl() noexcept
	{
		if (allocator != nullptr)
		{
			allocator->Release();
			allocator = nullptr;
		}
	}
};

namespace
{
	struct CategoryAggregation
	{
		RhiMemoryCategoryStats Stats;
		std::vector<ID3D12Heap*> UniqueHeaps;
	};

	std::uint64_t GetBudgetBytesForResidency(
	    RhiMemoryResidencyClass residencyClass,
	    const D3D12MA::Budget& localBudget,
	    const D3D12MA::Budget& nonLocalBudget) noexcept
	{
		switch (residencyClass)
		{
			case RhiMemoryResidencyClass::HostUpload:
			case RhiMemoryResidencyClass::HostReadback:
				return nonLocalBudget.BudgetBytes != 0 ? nonLocalBudget.BudgetBytes : localBudget.BudgetBytes;
			case RhiMemoryResidencyClass::DeviceLocal:
			case RhiMemoryResidencyClass::Transient:
			default:
				return localBudget.BudgetBytes;
		}
	}

	CategoryAggregation& FindOrCreateAggregation(
	    std::vector<CategoryAggregation>& aggregations,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    const D3D12MA::Budget& localBudget,
	    const D3D12MA::Budget& nonLocalBudget)
	{
		auto existing = std::find_if(
		    aggregations.begin(),
		    aggregations.end(),
		    [category, residencyClass](const CategoryAggregation& aggregation)
		    {
			    return aggregation.Stats.Category == category && aggregation.Stats.ResidencyClass == residencyClass;
		    });
		if (existing != aggregations.end())
		{
			return *existing;
		}

		CategoryAggregation aggregation;
		aggregation.Stats.Category = category;
		aggregation.Stats.ResidencyClass = residencyClass;
		aggregation.Stats.BudgetBytes = GetBudgetBytesForResidency(residencyClass, localBudget, nonLocalBudget);
		aggregations.push_back(std::move(aggregation));
		return aggregations.back();
	}

	void AddBlockReference(CategoryAggregation& aggregation, D3D12MA::Allocation* allocation) noexcept
	{
		if (allocation == nullptr)
		{
			return;
		}

		ID3D12Heap* const heap = allocation->GetHeap();
		if (heap == nullptr)
		{
			++aggregation.Stats.BlockCount;
			return;
		}

		if (std::find(aggregation.UniqueHeaps.begin(), aggregation.UniqueHeaps.end(), heap) == aggregation.UniqueHeaps.end())
		{
			aggregation.UniqueHeaps.push_back(heap);
			++aggregation.Stats.BlockCount;
		}
	}

	std::string WideStringToUtf8(const wchar_t* text)
	{
		if (text == nullptr || text[0] == L'\0')
		{
			return {};
		}

		const int requiredBytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (requiredBytes <= 1)
		{
			return {};
		}

		std::string result(static_cast<std::size_t>(requiredBytes - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), requiredBytes, nullptr, nullptr);
		return result;
	}
}  // namespace

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
}

D3D12GpuMemoryAllocator::~D3D12GpuMemoryAllocator() noexcept = default;

bool D3D12GpuMemoryAllocator::IsInitialized() const noexcept
{
	return m_impl != nullptr && m_impl->allocator != nullptr;
}

bool D3D12GpuMemoryAllocator::SupportsBudgetQueries() const noexcept
{
	return IsInitialized();
}

bool D3D12GpuMemoryAllocator::SupportsJsonDump() const noexcept
{
	return IsInitialized();
}

RhiMemoryUsageSnapshot D3D12GpuMemoryAllocator::CreateMemoryUsageSnapshot() const
{
	RhiMemoryUsageSnapshot snapshot;
	if (m_impl == nullptr || m_impl->allocator == nullptr)
	{
		return snapshot;
	}

	D3D12MA::Budget localBudget = {};
	D3D12MA::Budget nonLocalBudget = {};
	m_impl->allocator->GetBudget(&localBudget, &nonLocalBudget);

	D3D12MA::TotalStatistics totalStats = {};
	m_impl->allocator->CalculateStatistics(&totalStats);
	snapshot.AllocatorBackend = ERhiMemoryAllocatorBackend::D3D12Managed;
	snapshot.HasBudgetData = true;
	snapshot.HasAllocationDetails = true;
	snapshot.HasDelayedDestructionTracking = false;
	snapshot.TotalUsedBytes = totalStats.Total.Stats.AllocationBytes;
	snapshot.TotalAllocatedBytes = totalStats.Total.Stats.BlockBytes;
	snapshot.TotalBudgetBytes = localBudget.BudgetBytes + nonLocalBudget.BudgetBytes;
	snapshot.ApiUsageBytes = localBudget.UsageBytes + nonLocalBudget.UsageBytes;

	std::vector<CategoryAggregation> aggregations;
	{
		std::scoped_lock lock(m_impl->recordsMutex);
		snapshot.Allocations.reserve(m_impl->liveRecords.size());
		aggregations.reserve(m_impl->liveRecords.size());

		for (const D3D12GpuAllocationRecord* record : m_impl->liveRecords)
		{
			if (record == nullptr || record->Allocation == nullptr)
			{
				continue;
			}

			const std::uint64_t allocationBytes = record->Allocation->GetSize();
			if (record->Allocation->GetHeap() == nullptr)
			{
				snapshot.CommittedUsageBytes += allocationBytes;
			}
			else
			{
				snapshot.PlacedUsageBytes += allocationBytes;
			}
			if (record->ResidencyClass == RhiMemoryResidencyClass::Transient || record->Category == RhiMemoryCategory::TransientResource)
			{
				snapshot.TransientUsageBytes += allocationBytes;
			}
			CategoryAggregation& aggregation = FindOrCreateAggregation(
			    aggregations,
			    record->Category,
			    record->ResidencyClass,
			    localBudget,
			    nonLocalBudget);
			++aggregation.Stats.AllocationCount;
			if (record->Resource != nullptr)
			{
				++aggregation.Stats.ResourceCount;
			}
			aggregation.Stats.UsedBytes += allocationBytes;
			aggregation.Stats.AllocatedBytes += allocationBytes;
			AddBlockReference(aggregation, record->Allocation);

			snapshot.Allocations.push_back(RhiMemoryAllocationInfo{
			    .Category = record->Category,
			    .ResidencyClass = record->ResidencyClass,
			    .UsedBytes = allocationBytes,
			    .AllocatedBytes = allocationBytes,
			    .DebugName = record->DebugName});
		}

		for (const D3D12GpuHeapRecord* record : m_impl->liveHeapRecords)
		{
			if (record == nullptr || record->Allocation == nullptr)
			{
				continue;
			}

			const std::uint64_t allocationBytes = record->Allocation->GetSize();
			snapshot.PlacedUsageBytes += allocationBytes;
			if (record->ResidencyClass == RhiMemoryResidencyClass::Transient || record->Category == RhiMemoryCategory::TransientResource)
			{
				snapshot.TransientUsageBytes += allocationBytes;
			}
			CategoryAggregation& aggregation = FindOrCreateAggregation(
			    aggregations,
			    record->Category,
			    record->ResidencyClass,
			    localBudget,
			    nonLocalBudget);
			++aggregation.Stats.AllocationCount;
			aggregation.Stats.ResourceCount += record->AliasingResourceCount;
			aggregation.Stats.UsedBytes += allocationBytes;
			aggregation.Stats.AllocatedBytes += allocationBytes;
			AddBlockReference(aggregation, record->Allocation);

			snapshot.Allocations.push_back(RhiMemoryAllocationInfo{
			    .Category = record->Category,
			    .ResidencyClass = record->ResidencyClass,
			    .UsedBytes = allocationBytes,
			    .AllocatedBytes = allocationBytes,
			    .DebugName = record->DebugName});
		}
	}

	snapshot.CategoryStats.reserve(aggregations.size());
	for (const CategoryAggregation& aggregation : aggregations)
	{
		snapshot.CategoryStats.push_back(aggregation.Stats);
	}

	return snapshot;
}

bool D3D12GpuMemoryAllocator::WriteAllocatorJsonDump(const std::filesystem::path& outputPath, bool includeDetailedMap) const noexcept
{
	if (m_impl == nullptr || m_impl->allocator == nullptr || outputPath.empty())
	{
		return false;
	}

	WCHAR* statsString = nullptr;
	m_impl->allocator->BuildStatsString(&statsString, includeDetailedMap ? TRUE : FALSE);
	if (statsString == nullptr)
	{
		return false;
	}

	const std::string utf8Json = WideStringToUtf8(statsString);
	m_impl->allocator->FreeStatsString(statsString);
	if (utf8Json.empty())
	{
		return false;
	}

	std::error_code directoryError;
	const std::filesystem::path parentPath = outputPath.parent_path();
	if (!parentPath.empty())
	{
		std::filesystem::create_directories(parentPath, directoryError);
		if (directoryError)
		{
			return false;
		}
	}

	std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		return false;
	}

	output.write(utf8Json.data(), static_cast<std::streamsize>(utf8Json.size()));
	return output.good();
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

std::unique_ptr<D3D12GpuHeapRecord> D3D12GpuMemoryAllocator::CreateTransientHeap(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->allocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	allocationDesc.ExtraHeapFlags = ToTransientHeapFlags(pool);

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{};
	allocationInfo.SizeInBytes = sizeInBytes;
	allocationInfo.Alignment = alignment;

	D3D12MA::Allocation* allocation = nullptr;
	const HRESULT hr = m_impl->allocator->AllocateMemory(&allocationDesc, &allocationInfo, &allocation);
	if (FAILED(hr) || allocation == nullptr || allocation->GetHeap() == nullptr)
	{
		if (allocation != nullptr)
		{
			allocation->Release();
		}
		return {};
	}

	auto record = std::make_unique<D3D12GpuHeapRecord>();
	record->NativeHeap = allocation->GetHeap();
	record->Allocation = allocation;
	record->Category = RhiMemoryCategory::TransientResource;
	record->ResidencyClass = RhiMemoryResidencyClass::Transient;
	record->Owner = this;
	record->DebugName = std::wstring(debugName);
	if (!record->DebugName.empty())
	{
		record->NativeHeap->SetName(record->DebugName.c_str());
		record->Allocation->SetName(record->DebugName.c_str());
	}
	RegisterHeapRecord(*record);

	return record;
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateAliasingTexture(
    D3D12GpuHeapRecord& heap,
    std::uint64_t heapOffset,
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* optimizedClearValue,
    std::wstring_view debugName) noexcept
{
	return CreateAliasingResource(heap, heapOffset, resourceDesc, initialState, optimizedClearValue, debugName);
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateAliasingBuffer(
    D3D12GpuHeapRecord& heap,
    std::uint64_t heapOffset,
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    std::wstring_view debugName) noexcept
{
	return CreateAliasingResource(heap, heapOffset, resourceDesc, initialState, nullptr, debugName);
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
		std::size_t liveAllocationCount = 0;
		std::size_t liveHeapCount = 0;
		{
			std::scoped_lock lock(m_impl->recordsMutex);
			liveAllocationCount = m_impl->liveRecords.size();
			liveHeapCount = m_impl->liveHeapRecords.size();
		}
		SPDLOG_LOGGER_WARN(
		    g_d3d12MemoryLogger,
		    "D3D12 allocation failed: hr={} bytes={} category={} residency={} liveAllocations={} liveHeaps={} name='{}'",
		    Formatting::FormatPrefixedHexUInt32(static_cast<std::uint32_t>(hr)),
		    static_cast<std::uint64_t>(resourceDesc.Width),
		    RhiMemoryCategoryName(category),
		    RhiMemoryResidencyClassName(residencyClass),
		    liveAllocationCount,
		    liveHeapCount,
		    WideStringToUtf8(std::wstring(debugName).c_str()));
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
	record->Owner = this;
	record->DebugName = std::wstring(debugName);
	if (!record->DebugName.empty())
	{
		record->Resource->SetName(record->DebugName.c_str());
		record->Allocation->SetName(record->DebugName.c_str());
	}
	RegisterAllocationRecord(*record);

	return record;
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12GpuMemoryAllocator::CreateAliasingResource(
    D3D12GpuHeapRecord& heap,
    std::uint64_t heapOffset,
    const D3D12_RESOURCE_DESC& resourceDesc,
    D3D12_RESOURCE_STATES initialState,
    const D3D12_CLEAR_VALUE* optimizedClearValue,
    std::wstring_view debugName) noexcept
{
	if (m_impl == nullptr || m_impl->allocator == nullptr || heap.Allocation == nullptr)
	{
		return {};
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	const HRESULT hr = m_impl->allocator->CreateAliasingResource(
	    heap.Allocation,
	    heapOffset,
	    &resourceDesc,
	    initialState,
	    optimizedClearValue,
	    IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
	if (FAILED(hr) || resource == nullptr)
	{
		return {};
	}

	auto record = std::make_unique<D3D12GpuAllocationRecord>();
	record->Resource = std::move(resource);
	record->Allocation = heap.Allocation;
	record->Allocation->AddRef();
	record->ParentHeap = &heap;
	record->Category = RhiMemoryCategory::TransientResource;
	record->ResidencyClass = RhiMemoryResidencyClass::Transient;
	record->DebugName = std::wstring(debugName);
	++heap.AliasingResourceCount;
	if (!record->DebugName.empty())
	{
		record->Resource->SetName(record->DebugName.c_str());
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

D3D12_HEAP_FLAGS D3D12GpuMemoryAllocator::ToTransientHeapFlags(RhiTransientAllocationPool pool) noexcept
{
	switch (pool)
	{
		case RhiTransientAllocationPool::Buffer:
			return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		case RhiTransientAllocationPool::Depth:
		case RhiTransientAllocationPool::Color:
		default:
			return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
	}
}

void D3D12GpuMemoryAllocator::RegisterAllocationRecord(D3D12GpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->recordsMutex);
	if (std::find(m_impl->liveRecords.begin(), m_impl->liveRecords.end(), &record) == m_impl->liveRecords.end())
	{
		m_impl->liveRecords.push_back(&record);
	}
}

void D3D12GpuMemoryAllocator::UnregisterAllocationRecord(D3D12GpuAllocationRecord& record) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->recordsMutex);
	auto eraseBegin = std::remove(m_impl->liveRecords.begin(), m_impl->liveRecords.end(), &record);
	m_impl->liveRecords.erase(eraseBegin, m_impl->liveRecords.end());
}

void D3D12GpuMemoryAllocator::RegisterHeapRecord(D3D12GpuHeapRecord& record) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->recordsMutex);
	if (std::find(m_impl->liveHeapRecords.begin(), m_impl->liveHeapRecords.end(), &record) == m_impl->liveHeapRecords.end())
	{
		m_impl->liveHeapRecords.push_back(&record);
	}
}

void D3D12GpuMemoryAllocator::UnregisterHeapRecord(D3D12GpuHeapRecord& record) noexcept
{
	if (m_impl == nullptr)
	{
		return;
	}

	std::scoped_lock lock(m_impl->recordsMutex);
	auto eraseBegin = std::remove(m_impl->liveHeapRecords.begin(), m_impl->liveHeapRecords.end(), &record);
	m_impl->liveHeapRecords.erase(eraseBegin, m_impl->liveHeapRecords.end());
}
