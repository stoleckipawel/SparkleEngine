#include "PCH.h"

#include "D3D12/Memory/D3D12RecordingResourceTable.h"

#include "D3D12/Memory/D3D12GpuAllocation.h"

#include <algorithm>
#include <cassert>
#include <vector>

struct D3D12RecordingResourceTable::ResourceEntry final
{
	std::uintptr_t ResourceValue = 0;
	D3D12GpuAllocationRecord* Record = nullptr;
};

struct D3D12RecordingResourceTable::ReadView final
{
	~ReadView() noexcept;

	std::vector<ResourceEntry> Resources;
};

D3D12RecordingResourceTable::ReadView::~ReadView() noexcept
{
	for (const ResourceEntry& entry : Resources)
	{
		if (entry.Record != nullptr)
		{
			D3D12RecordingResourceTable::ReleaseReference(*entry.Record);
		}
	}
}

D3D12RecordingResourceTable::D3D12RecordingResourceTable() noexcept = default;
D3D12RecordingResourceTable::~D3D12RecordingResourceTable() noexcept = default;

void D3D12RecordingResourceTable::Publish(
    std::span<D3D12GpuAllocationRecord* const> records) noexcept
{
	std::shared_ptr<ReadView> readView = BuildReadView(records);
	m_readView.store(
	    std::shared_ptr<const ReadView>(std::move(readView)),
	    std::memory_order_release);
}

D3D12RecordingResourceUseToken D3D12RecordingResourceTable::Retain(
    RhiResourceHandle resource) const noexcept
{
	const std::shared_ptr<const ReadView> readView =
	    m_readView.load(std::memory_order_acquire);
	if (readView == nullptr)
	{
		return {};
	}

	const ResourceEntry* const entry = FindResource(*readView, resource);
	return entry != nullptr && entry->Record != nullptr
	           ? Retain(*entry->Record)
	           : D3D12RecordingResourceUseToken{};
}

D3D12RecordingResourceUseToken D3D12RecordingResourceTable::Retain(
    D3D12GpuAllocationRecord& record) const noexcept
{
	RetainReference(record);

	D3D12RecordingResourceUseToken use;
	use.m_value = reinterpret_cast<std::uintptr_t>(&record);
	return use;
}

void D3D12RecordingResourceTable::Release(
    D3D12RecordingResourceUseToken use,
    RhiSubmissionToken submissionToken) const noexcept
{
	if (!use)
	{
		return;
	}

	auto* const record =
	    reinterpret_cast<D3D12GpuAllocationRecord*>(use.m_value);
	ReleaseReference(*record, submissionToken);
}

std::shared_ptr<D3D12RecordingResourceTable::ReadView>
D3D12RecordingResourceTable::BuildReadView(
    std::span<D3D12GpuAllocationRecord* const> records)
{
	auto readView = std::make_shared<ReadView>();
	readView->Resources.reserve(records.size());

	for (D3D12GpuAllocationRecord* record : records)
	{
		if (record == nullptr ||
		    record->PendingRelease ||
		    record->Resource == nullptr)
		{
			continue;
		}

		RetainReference(*record);
		readView->Resources.push_back(ResourceEntry{
		    .ResourceValue = reinterpret_cast<std::uintptr_t>(record->Resource.Get()),
		    .Record = record});
	}

	std::ranges::sort(
	    readView->Resources,
	    {},
	    &ResourceEntry::ResourceValue);
	return readView;
}

const D3D12RecordingResourceTable::ResourceEntry*
D3D12RecordingResourceTable::FindResource(
    const ReadView& readView,
    RhiResourceHandle resource) noexcept
{
	if (!resource)
	{
		return nullptr;
	}

	const std::uintptr_t resourceValue =
	    reinterpret_cast<std::uintptr_t>(resource.Value);
	const auto found = std::ranges::lower_bound(
	    readView.Resources,
	    resourceValue,
	    {},
	    &ResourceEntry::ResourceValue);
	return found != readView.Resources.end() &&
	               found->ResourceValue == resourceValue
	           ? &*found
	           : nullptr;
}

void D3D12RecordingResourceTable::RetainReference(
    D3D12GpuAllocationRecord& record) noexcept
{
	record.RecordingReferenceCount.fetch_add(
	    1,
	    std::memory_order_relaxed);
	if (record.ParentHeap != nullptr)
	{
		record.ParentHeap->RecordingReferenceCount.fetch_add(
		    1,
		    std::memory_order_relaxed);
	}
}

void D3D12RecordingResourceTable::ReleaseReference(
    D3D12GpuAllocationRecord& record) noexcept
{
	const std::uint32_t previousReferences =
	    record.RecordingReferenceCount.fetch_sub(
	        1,
	        std::memory_order_relaxed);
	assert(previousReferences != 0);

	if (record.ParentHeap != nullptr)
	{
		const std::uint32_t previousHeapReferences =
		    record.ParentHeap->RecordingReferenceCount.fetch_sub(
		        1,
		        std::memory_order_relaxed);
		assert(previousHeapReferences != 0);
	}
}

void D3D12RecordingResourceTable::ReleaseReference(
    D3D12GpuAllocationRecord& record,
    RhiSubmissionToken submissionToken) noexcept
{
	record.LastUse.MarkUsed(submissionToken);
	if (record.ParentHeap != nullptr)
	{
		record.ParentHeap->LastUse.MarkUsed(submissionToken);
	}

	ReleaseReference(record);
}
