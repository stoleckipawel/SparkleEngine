#pragma once

#include "RhiBackendApi.h"
#include "../Formats/PixelFormat.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Shaders/CookedShaderPackage.h"

#include <array>
#include <cstdint>
#include <string>

enum class ERhiDescriptorModel : std::uint8_t
{
	Unknown = 0,
	DescriptorTables,
	DescriptorSets,
};

enum class ERhiMemoryAllocatorBackend : std::uint8_t
{
	Unknown = 0,
	D3D12Managed,
	VulkanManaged,
};

struct RhiBindingLimits
{
	std::uint32_t MaxDescriptorSets = 0;
	std::uint32_t MaxShaderResourceDescriptors = 0;
	std::uint32_t MaxSamplerDescriptors = 0;
	std::uint32_t MaxDescriptorTableEntries = 0;
	std::uint32_t MaxPushConstantBytes = 0;
};

struct RhiUploadReadbackCapabilities
{
	bool SupportsBufferUpload = false;
	bool SupportsTextureUpload = false;
	bool SupportsReadback = false;
};

struct RhiQueueCapabilities
{
	bool SupportsGraphics = false;
	bool SupportsCompute = false;
	bool SupportsCopy = false;
};

enum class ERhiExternalFeatureBridgeKind : std::uint8_t
{
	None = 0,
	D3D12NativeDevice,
	VulkanManualFunctionPointers,
	VulkanInterposer,
};

struct RhiAdapterIdentity
{
	std::string Name;
	std::string DriverDescription;
	std::uint32_t VendorId = 0;
	std::uint32_t DeviceId = 0;
};

struct RhiExternalFeatureInteropCapabilities
{
	// Provider-neutral facts used by renderer-owned external feature providers.
	//
	// Keep vendor SDK policy out of RHI. RHI may report native handle availability,
	// queue/command-list readiness, resource interop, and backend bridge shape; it
	// must not decide whether any concrete provider is enabled.
	ERhiExternalFeatureBridgeKind BridgeKind = ERhiExternalFeatureBridgeKind::None;
	RhiAdapterIdentity Adapter;
	bool ExposesNativeDevice = false;
	bool ExposesNativeGraphicsQueue = false;
	bool ExposesNativeGraphicsCommandList = false;
	bool ExposesNativeResources = false;
	bool SupportsExplicitResourceStates = false;
	bool SupportsExternalProviderEvaluation = false;
	bool SupportsRuntimeProviderChecks = false;
	bool VulkanHasInstanceHandle = false;
	bool VulkanHasPhysicalDeviceHandle = false;
	bool VulkanHasDeviceHandle = false;
	bool VulkanHasGraphicsQueueHandle = false;
	bool VulkanHasGraphicsQueueFamilyIndex = false;
	bool VulkanManualFunctionPointerHookingReady = false;
	bool VulkanInterposerRequired = false;
};

struct RhiFormatSupport
{
	PixelFormat Format = PixelFormat::Unknown;
	bool SupportsTexture = false;
	bool SupportsShaderResource = false;
	bool SupportsUnorderedAccess = false;
	bool SupportsRenderTarget = false;
	bool SupportsDepthStencil = false;
};

inline constexpr std::array<PixelFormat, 5> kRhiCapabilityPixelFormats = {
	PixelFormat::R8G8B8A8_UNorm,
	PixelFormat::B8G8R8A8_UNorm,
	PixelFormat::R16G16B16A16_Float,
	PixelFormat::D24_UNorm_S8_UInt,
	PixelFormat::R32_Float,
};

struct RhiCapabilities
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	CookedShaderBinaryFormat RequiredShaderBinaryFormat = CookedShaderBinaryFormat::Dxil;
	ERhiDescriptorModel DescriptorModel = ERhiDescriptorModel::Unknown;
	RhiBindingLimits BindingLimits;
	RhiUploadReadbackCapabilities UploadReadback;
	std::array<RhiFormatSupport, kRhiCapabilityPixelFormats.size()> FormatSupport = {};
	bool SupportsTimestampQueries = false;
	RhiRayTracingCapabilities RayTracing;
	bool SupportsMeshShaders = false;
	bool SupportsTaskShaders = false;
	RhiQueueCapabilities Queues;
	bool SupportsPresent = false;
	ERhiMemoryAllocatorBackend MemoryAllocator = ERhiMemoryAllocatorBackend::Unknown;
	RhiExternalFeatureInteropCapabilities ExternalFeatureInterop;

	const RhiFormatSupport* FindFormatSupport(PixelFormat format) const noexcept
	{
		for (const RhiFormatSupport& support : FormatSupport)
		{
			if (support.Format == format)
			{
				return &support;
			}
		}

		return nullptr;
	}
};

constexpr const char* RhiDescriptorModelToString(ERhiDescriptorModel model) noexcept
{
	switch (model)
	{
		case ERhiDescriptorModel::DescriptorTables:
			return "DescriptorTables";
		case ERhiDescriptorModel::DescriptorSets:
			return "DescriptorSets";
		case ERhiDescriptorModel::Unknown:
		default:
			return "Unknown";
	}
}

constexpr const char* RhiMemoryAllocatorBackendToString(ERhiMemoryAllocatorBackend backend) noexcept
{
	switch (backend)
	{
		case ERhiMemoryAllocatorBackend::D3D12Managed:
			return "D3D12Managed";
		case ERhiMemoryAllocatorBackend::VulkanManaged:
			return "VulkanManaged";
		case ERhiMemoryAllocatorBackend::Unknown:
		default:
			return "Unknown";
	}
}

constexpr const char* RhiExternalFeatureBridgeKindToString(ERhiExternalFeatureBridgeKind kind) noexcept
{
	switch (kind)
	{
		case ERhiExternalFeatureBridgeKind::D3D12NativeDevice:
			return "D3D12NativeDevice";
		case ERhiExternalFeatureBridgeKind::VulkanManualFunctionPointers:
			return "VulkanManualFunctionPointers";
		case ERhiExternalFeatureBridgeKind::VulkanInterposer:
			return "VulkanInterposer";
		case ERhiExternalFeatureBridgeKind::None:
		default:
			return "None";
	}
}
