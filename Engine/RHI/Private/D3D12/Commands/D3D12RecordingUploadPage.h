#pragma once

#include "Resources/RhiResourceDesc.h"

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <thread>
#include <wrl/client.h>

class D3D12Rhi;
struct D3D12GpuAllocationRecord;

class D3D12RecordingUploadPage final
{
public:
	D3D12RecordingUploadPage() noexcept;
	~D3D12RecordingUploadPage() noexcept;

	D3D12RecordingUploadPage(const D3D12RecordingUploadPage&) = delete;
	D3D12RecordingUploadPage& operator=(const D3D12RecordingUploadPage&) = delete;
	D3D12RecordingUploadPage(D3D12RecordingUploadPage&&) = delete;
	D3D12RecordingUploadPage& operator=(D3D12RecordingUploadPage&&) = delete;

	void Initialize(D3D12Rhi& rhi, std::uint64_t capacityInBytes, const wchar_t* debugName);
	void BeginRecording() noexcept;
	void EndRecording() noexcept;
	void Reset() noexcept;
	RhiGpuVirtualAddress AllocateAndCopy(const void* data, std::uint32_t sizeInBytes);

	std::uint64_t GetCapacityInBytes() const noexcept { return m_capacityInBytes; }
	std::uint64_t GetUsedBytes() const noexcept { return m_offset; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	std::unique_ptr<D3D12GpuAllocationRecord> m_allocation;
	std::uint8_t* m_cpuBase = nullptr;
	RhiGpuVirtualAddress m_gpuBase = 0;
	std::thread::id m_recordingThread;
	std::uint64_t m_capacityInBytes = 0;
	std::uint64_t m_offset = 0;
};
