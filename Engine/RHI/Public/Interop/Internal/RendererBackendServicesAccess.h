#pragma once

#include "../RendererBackendServices.h"

class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12SamplerLibrary;
class D3D12SwapChain;

namespace Rhi::Internal
{
	struct SPARKLE_RHI_API RendererBackendServicesAccess final
	{
		static D3D12Rhi& GetRhi(RendererBackendServices& services) noexcept;
		static D3D12DescriptorHeapManager& GetDescriptorHeapManager(RendererBackendServices& services) noexcept;
		static D3D12SwapChain& GetSwapChain(RendererBackendServices& services) noexcept;
		static D3D12ConstantBufferManager& GetConstantBufferManager(RendererBackendServices& services) noexcept;
		static D3D12SamplerLibrary& GetSamplerLibrary(RendererBackendServices& services) noexcept;
	};
}