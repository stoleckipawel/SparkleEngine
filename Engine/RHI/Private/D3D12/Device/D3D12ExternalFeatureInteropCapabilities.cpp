#include "PCH.h"

#include "D3D12/Device/D3D12ExternalFeatureInteropCapabilities.h"

#include "D3D12/Device/D3D12Rhi.h"

#include <d3d12.h>
#include <cstring>
#include <string>
#include <string_view>

namespace
{
	std::string NarrowAdapterDescription(std::wstring_view value)
	{
		std::string result;
		result.reserve(value.size());
		for (const wchar_t ch : value)
		{
			if (ch == L'\0')
			{
				break;
			}
			result.push_back(ch <= 0x7f ? static_cast<char>(ch) : '?');
		}
		return result;
	}

	RhiAdapterIdentity BuildD3D12AdapterIdentity(const D3D12Rhi* rhi) noexcept
	{
		if (rhi == nullptr || rhi->GetAdapter() == nullptr)
		{
			return {};
		}

		DXGI_ADAPTER_DESC1 adapterDesc{};
		if (FAILED(rhi->GetAdapter()->GetDesc1(&adapterDesc)))
		{
			return {};
		}

		RhiAdapterIdentity identity{
		    .Name = NarrowAdapterDescription(adapterDesc.Description),
		    .DriverDescription = "DXGI",
		    .VendorId = adapterDesc.VendorId,
		    .DeviceId = adapterDesc.DeviceId};
		static_assert(sizeof(adapterDesc.AdapterLuid) <= identity.NativeLuid.size());
		std::memcpy(identity.NativeLuid.data(), &adapterDesc.AdapterLuid, sizeof(adapterDesc.AdapterLuid));
		identity.NativeLuidSizeInBytes = static_cast<std::uint32_t>(sizeof(adapterDesc.AdapterLuid));
		return identity;
	}
}

RhiExternalFeatureInteropCapabilities BuildD3D12ExternalFeatureInteropCapabilities(
    const D3D12Rhi* rhi,
    bool hasGraphicsCommandList) noexcept
{
	RhiExternalFeatureInteropCapabilities capabilities{};
	capabilities.BridgeKind = ERhiExternalFeatureBridgeKind::D3D12NativeDevice;
	capabilities.Adapter = BuildD3D12AdapterIdentity(rhi);
	capabilities.ExposesNativeDevice = rhi != nullptr && rhi->GetDevice() != nullptr;
	capabilities.ExposesNativeGraphicsQueue = rhi != nullptr && rhi->GetCommandQueue() != nullptr;
	capabilities.ExposesNativeGraphicsCommandList = hasGraphicsCommandList;
	capabilities.ExposesNativeResources = true;
	capabilities.SupportsExplicitResourceStates = true;
	capabilities.SupportsExternalProviderEvaluation =
	    capabilities.ExposesNativeDevice && capabilities.ExposesNativeGraphicsQueue && capabilities.ExposesNativeGraphicsCommandList;
	capabilities.SupportsRuntimeProviderChecks = capabilities.SupportsExternalProviderEvaluation;
	return capabilities;
}
