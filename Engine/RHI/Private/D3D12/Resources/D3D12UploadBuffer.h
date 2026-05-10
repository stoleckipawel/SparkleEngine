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
	static ComPtr<ID3D12Resource2> Upload(D3D12Rhi& rhi, const void* data, size_t dataSize);
};
