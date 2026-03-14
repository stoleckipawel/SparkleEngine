#include "PCH.h"
#include "D3D12UploadBuffer.h"
#include "D3D12Rhi.h"
#include "Log.h"
#include <cstring>

ComPtr<ID3D12Resource2> D3D12UploadBuffer::Upload(D3D12Rhi& rhi, const void* data, size_t dataSize)
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
	ComPtr<ID3D12Resource2> uploadBuffer;
	CD3DX12_HEAP_PROPERTIES heapUploadProperties(D3D12_HEAP_TYPE_UPLOAD);
	CHECK(rhi.GetDevice()->CreateCommittedResource(
	    &heapUploadProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf())));

	uploadBuffer->SetName(L"RHI_UploadBuffer");

	void* mappedData = nullptr;
	D3D12_RANGE readRange = {0, 0};
	CHECK(uploadBuffer->Map(0, &readRange, &mappedData));

	if (dataSize > 0 && data != nullptr && mappedData != nullptr)
	{
		std::memcpy(mappedData, data, dataSize);
	}
	uploadBuffer->Unmap(0, nullptr);

	return uploadBuffer;
}