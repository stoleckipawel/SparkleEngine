// ============================================================================
// D3D12UploadBuffer.h
// ----------------------------------------------------------------------------
// Utility for uploading small data blobs to GPU memory.
//
#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstddef>
#include <type_traits>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

class D3D12UploadBuffer
{
  public:
	// Uploads the given data to a newly created upload-heap buffer and returns
	// a ComPtr to the GPU resource. `data` must point to at least `dataSize`
	// bytes. `dataSize` is size in bytes.
	static ComPtr<ID3D12Resource2> Upload(D3D12Rhi& rhi, const void* data, size_t dataSize);
};
