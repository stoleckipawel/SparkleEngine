#pragma once

#include "RhiBackendApi.h"
#include "../Commands/RhiQueueCapabilities.h"
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

enum class ERhiPresentationThrottle : std::uint8_t
{
	None = 0,
	FrameLatencyWaitableObject,
	SwapChainImageAcquisition,
};

enum class ERhiBackendVersionSemantic : std::uint8_t
{
	Unknown = 0,
	ApiVersion,
	FeatureLevel,
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

struct RhiPresentationCapabilities
{
	std::uint32_t BackBufferCount = 0;
	std::uint32_t MaximumFramesInFlight = 0;
	ERhiPresentationThrottle Throttle = ERhiPresentationThrottle::None;
};

struct RhiDescriptorIndexingCapabilities
{
	bool SupportsSampledImageArrayNonUniformIndexing = false;
	bool SupportsPartiallyBoundDescriptorArrays = false;
};

struct RhiBackendVersionInfo
{
	ERhiBackendVersionSemantic Semantic = ERhiBackendVersionSemantic::Unknown;
	std::uint32_t Major = 0;
	std::uint32_t Minor = 0;
	std::uint32_t Patch = 0;
	std::uint32_t PackedValue = 0;

	constexpr bool IsKnown() const noexcept { return Semantic != ERhiBackendVersionSemantic::Unknown; }
};

struct RhiBackendDiagnosticsSupport
{
	bool ValidationEnabled = false;
	bool SupportsDebugLayer = false;
	bool SupportsObjectNames = false;
	bool SupportsGpuEvents = false;
	bool SupportsTimestampQueries = false;
	bool SupportsDebugMessages = false;
	bool SupportsLiveObjectReports = false;
	bool SupportsCrashDiagnostics = false;
};

struct RhiBackendMemorySupport
{
	bool SupportsMemoryDiagnostics = false;
	bool SupportsBudgetQueries = false;
	bool SupportsDelayedDestructionTracking = false;
	bool SupportsResidencyPressure = false;
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
	std::array<std::uint8_t, 8> NativeLuid = {};
	std::uint32_t NativeLuidSizeInBytes = 0;
};

struct RhiExternalFeatureInteropCapabilities
{
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

inline constexpr std::array<PixelFormat, 23> kRhiCapabilityPixelFormats = {
	PixelFormat::R32G32B32A32_Float,
	PixelFormat::R16G16B16A16_Float,
	PixelFormat::R8G8B8A8_UNorm,
	PixelFormat::R8G8B8A8_UNorm_Srgb,
	PixelFormat::R16G16_Float,
	PixelFormat::D32_Float,
	PixelFormat::R32_Float,
	PixelFormat::D24_UNorm_S8_UInt,
	PixelFormat::BC1_UNorm,
	PixelFormat::BC1_UNorm_Srgb,
	PixelFormat::BC2_UNorm,
	PixelFormat::BC2_UNorm_Srgb,
	PixelFormat::BC3_UNorm,
	PixelFormat::BC3_UNorm_Srgb,
	PixelFormat::BC4_UNorm,
	PixelFormat::BC4_SNorm,
	PixelFormat::BC5_UNorm,
	PixelFormat::BC5_SNorm,
	PixelFormat::B8G8R8A8_UNorm,
	PixelFormat::B8G8R8A8_UNorm_Srgb,
	PixelFormat::BC6H_UF16,
	PixelFormat::BC7_UNorm,
	PixelFormat::BC7_UNorm_Srgb,
};

struct RhiCapabilities
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	CookedShaderBinaryFormat RuntimeShaderBinaryFormat = CookedShaderBinaryFormat::Dxil;
	RhiBackendVersionInfo BackendVersion;
	ERhiDescriptorModel DescriptorModel = ERhiDescriptorModel::Unknown;
	RhiBindingLimits BindingLimits;
	RhiDescriptorIndexingCapabilities DescriptorIndexing;
	RhiUploadReadbackCapabilities UploadReadback;
	RhiPresentationCapabilities Presentation;
	std::array<RhiFormatSupport, kRhiCapabilityPixelFormats.size()> FormatSupport = {};
	RhiBackendDiagnosticsSupport Diagnostics;
	RhiRayTracingCapabilities RayTracing;
	bool SupportsMeshShaders = false;
	bool SupportsTaskShaders = false;
	RhiQueueCapabilities Queues;
	bool SupportsPresent = false;
	ERhiMemoryAllocatorBackend MemoryAllocator = ERhiMemoryAllocatorBackend::Unknown;
	RhiBackendMemorySupport MemorySupport;
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

constexpr const char* RhiBackendVersionSemanticToString(ERhiBackendVersionSemantic semantic) noexcept
{
	switch (semantic)
	{
		case ERhiBackendVersionSemantic::ApiVersion:
			return "ApiVersion";
		case ERhiBackendVersionSemantic::FeatureLevel:
			return "FeatureLevel";
		case ERhiBackendVersionSemantic::Unknown:
		default:
			return "Unknown";
	}
}
