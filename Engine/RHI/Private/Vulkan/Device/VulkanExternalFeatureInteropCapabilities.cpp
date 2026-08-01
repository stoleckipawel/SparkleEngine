#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanExternalFeatureInteropCapabilities.h"

#include "Vulkan/Device/VulkanRhi.h"

class VulkanExternalFeatureAdapterIdentity final
{
  public:
	static RhiAdapterIdentity BuildVulkanAdapterIdentity(const VulkanRhi* rhi)
	{
		if (rhi == nullptr)
		{
			return {};
		}

		const VulkanAdapterInfo& adapter = rhi->GetAdapterInfo();
		return RhiAdapterIdentity{
		    .Name = adapter.Name,
		    .DriverDescription = adapter.Driver,
		    .VendorId = adapter.VendorId,
		    .DeviceId = adapter.DeviceId};
	}
};

RhiExternalFeatureInteropCapabilities BuildVulkanExternalFeatureInteropCapabilities(
    const VulkanRhi* rhi,
    bool hasGraphicsCommandContext) noexcept
{
	RhiExternalFeatureInteropCapabilities capabilities{};
	capabilities.BridgeKind = ERhiExternalFeatureBridgeKind::None;
	capabilities.Adapter = VulkanExternalFeatureAdapterIdentity::BuildVulkanAdapterIdentity(rhi);
	capabilities.ExposesNativeDevice = rhi != nullptr && rhi->GetDevice() != VK_NULL_HANDLE;
	capabilities.ExposesNativeGraphicsQueue = rhi != nullptr && rhi->GetGraphicsQueue() != VK_NULL_HANDLE;
	capabilities.ExposesNativeGraphicsCommandList = hasGraphicsCommandContext;
	capabilities.ExposesNativeResources = true;
	capabilities.SupportsExplicitResourceStates = true;
	capabilities.SupportsExternalProviderEvaluation = false;
	capabilities.SupportsRuntimeProviderChecks = false;
	return capabilities;
}
