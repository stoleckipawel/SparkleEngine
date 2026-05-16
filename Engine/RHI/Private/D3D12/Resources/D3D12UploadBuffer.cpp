#include "PCH.h"
#include "D3D12/Resources/D3D12UploadBuffer.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include <cstring>

static const auto g_d3d12UploadBufferLogger = Logging::GetOrCreateLogger("RHI.D3D12.UploadBuffer");

std::unique_ptr<D3D12GpuAllocationRecord> D3D12UploadBuffer::Upload(D3D12Rhi& rhi, const void* data, size_t dataSize)
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Width = static_cast<UINT64>(dataSize);
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	std::unique_ptr<D3D12GpuAllocationRecord> uploadBuffer = rhi.GetMemoryAllocator().CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    L"RHI_UploadBuffer");
	if (uploadBuffer == nullptr || uploadBuffer->Resource == nullptr)
	{
		Diagnostics::Fail(g_d3d12UploadBufferLogger, __FILE__, __LINE__, "D3D12UploadBuffer: failed to allocate upload buffer.");
	}

	void* mappedData = nullptr;
	D3D12_RANGE readRange = {0, 0};
	CHECK(uploadBuffer->Resource->Map(0, &readRange, &mappedData));
	uploadBuffer->IsMapped = true;
	uploadBuffer->CpuMappedAddress = mappedData;

	if (dataSize > 0 && data != nullptr && mappedData != nullptr)
	{
		std::memcpy(mappedData, data, dataSize);
	}
	uploadBuffer->Resource->Unmap(0, nullptr);
	uploadBuffer->IsMapped = false;
	uploadBuffer->CpuMappedAddress = nullptr;

	return uploadBuffer;
}