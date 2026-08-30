#include "PCH.h"

#include "D3D12/Commands/D3D12RecordingUploadPage.h"

#include "Core/Public/Math/MathUtils.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

D3D12RecordingUploadPage::D3D12RecordingUploadPage() noexcept = default;

D3D12RecordingUploadPage::~D3D12RecordingUploadPage() noexcept = default;

void D3D12RecordingUploadPage::Initialize(D3D12Rhi& rhi, std::uint64_t capacityInBytes, const wchar_t* debugName)
{
	assert(capacityInBytes != 0);

	const D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(capacityInBytes);
	m_allocation = rhi.GetMemoryAllocator().CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::ConstantBuffer,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName);
	if (m_allocation == nullptr || m_allocation->Resource == nullptr)
	{
		throw std::runtime_error("Failed to allocate a D3D12 command-recording upload page.");
	}

	m_resource = m_allocation->Resource;
	D3D12_RANGE readRange{0, 0};
	const HRESULT mapResult = m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_cpuBase));
	if (FAILED(mapResult))
	{
		throw std::runtime_error("Failed to map a D3D12 command-recording upload page.");
	}

	m_allocation->IsMapped = true;
	m_allocation->CpuMappedAddress = m_cpuBase;
	m_gpuBase = m_resource->GetGPUVirtualAddress();
	m_capacityInBytes = capacityInBytes;
}

void D3D12RecordingUploadPage::BeginRecording() noexcept
{
	const std::thread::id thread = std::this_thread::get_id();
	assert(m_recordingThread == std::thread::id{} || m_recordingThread == thread);
	m_recordingThread = thread;
}

void D3D12RecordingUploadPage::EndRecording() noexcept
{
	assert(m_recordingThread == std::this_thread::get_id());
	m_recordingThread = {};
}

void D3D12RecordingUploadPage::Reset() noexcept
{
	m_recordingThread = {};
	m_offset = 0;
}

RhiGpuVirtualAddress D3D12RecordingUploadPage::AllocateAndCopy(const void* data, std::uint32_t sizeInBytes)
{
	assert(data != nullptr);
	assert(sizeInBytes != 0);
	assert(m_recordingThread == std::this_thread::get_id());

	const std::uint64_t alignedOffset = MathUtils::AlignUp(m_offset, 256ull);
	const std::uint64_t alignedSize = MathUtils::AlignUp(static_cast<std::uint64_t>(sizeInBytes), 256ull);
	if (alignedOffset + alignedSize > m_capacityInBytes)
	{
		throw std::bad_alloc();
	}

	std::memcpy(m_cpuBase + alignedOffset, data, sizeInBytes);
	m_offset = alignedOffset + alignedSize;
	return m_gpuBase + alignedOffset;
}
