#pragma once

#include "Commands/RhiQueue.h"
#include "D3D12/Memory/D3D12RecordingResourceUseToken.h"
#include "Resources/RhiResourceHandles.h"

#include <atomic>
#include <memory>
#include <span>

struct D3D12GpuAllocationRecord;

class D3D12RecordingResourceTable final
{
  public:
	D3D12RecordingResourceTable() noexcept;
	~D3D12RecordingResourceTable() noexcept;

	D3D12RecordingResourceTable(const D3D12RecordingResourceTable&) = delete;
	D3D12RecordingResourceTable& operator=(const D3D12RecordingResourceTable&) = delete;
	D3D12RecordingResourceTable(D3D12RecordingResourceTable&&) = delete;
	D3D12RecordingResourceTable& operator=(D3D12RecordingResourceTable&&) = delete;

  private:
	friend class D3D12GpuMemoryAllocator;

	void Publish(std::span<D3D12GpuAllocationRecord* const> records) noexcept;
	D3D12RecordingResourceUseToken Retain(RhiResourceHandle resource) const noexcept;
	D3D12RecordingResourceUseToken Retain(D3D12GpuAllocationRecord& record) const noexcept;
	void Release(
	    D3D12RecordingResourceUseToken use,
	    RhiSubmissionToken submissionToken) const noexcept;

	struct ResourceEntry;
	struct ReadView;

	static std::shared_ptr<ReadView> BuildReadView(
	    std::span<D3D12GpuAllocationRecord* const> records);
	static const ResourceEntry* FindResource(
	    const ReadView& readView,
	    RhiResourceHandle resource) noexcept;
	static void RetainReference(D3D12GpuAllocationRecord& record) noexcept;
	static void ReleaseReference(D3D12GpuAllocationRecord& record) noexcept;
	static void ReleaseReference(
	    D3D12GpuAllocationRecord& record,
	    RhiSubmissionToken submissionToken) noexcept;

	std::atomic<std::shared_ptr<const ReadView>> m_readView;
};
