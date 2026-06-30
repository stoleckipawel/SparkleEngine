#include "../PCH.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"

#include "Frame/Core/FrameContext.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "SceneData/MaterialTextureTableCapability.h"

namespace
{
	const RayTracingCapabilityReport* ResolveCapabilityReport(const RenderRayTracingPassServices* rayTracingServices) noexcept
	{
		if (rayTracingServices == nullptr)
		{
			return nullptr;
		}
		if (rayTracingServices->CapabilityReport != nullptr)
		{
			return rayTracingServices->CapabilityReport;
		}
		return rayTracingServices->Scene != nullptr ? &rayTracingServices->Scene->GetCapabilities() : nullptr;
	}

	bool HasMaterialTextureTable(const FrameContext& frame) noexcept
	{
		const RenderBindingSet* materialTextureTable = frame.sceneData.materialTextureTable;
		if (!frame.sceneData.materialTextureTableValid || materialTextureTable == nullptr || !*materialTextureTable)
		{
			return false;
		}

		const std::uint32_t descriptorCount = frame.sceneData.materialTextureTableDescriptorCount;
		return descriptorCount != 0u && descriptorCount <= MaterialTextureTableFixedCapacity &&
		       materialTextureTable->GetDescriptorCount() >= descriptorCount;
	}
}

namespace RayTracingPassCapabilityQuery
{
	RayTracingPassCapabilities Build(
	    const FrameContext& frame,
	    const RenderRayTracingPassServices* rayTracingServices) noexcept
	{
		const RayTracingCapabilityReport* capabilityReport = ResolveCapabilityReport(rayTracingServices);
		RayTracingPassCapabilities result{
		    .BoundSceneTlasAvailable = frame.rayTracingScene.HasBoundTlas(),
		    .SceneTlasShaderAccessMode = frame.rayTracingScene.TlasShaderAccessMode,
		    .TriangleMaterialDataAvailable = frame.rayTracingHitData.IsValid(),
		    .HitDataAvailable = frame.rayTracingHitData.IsValid() && frame.meshInstances.IsValid(),
		    .MaterialTextureTableAvailable = HasMaterialTextureTable(frame),
		    .MaterialTextureTableDescriptorCount = frame.sceneData.materialTextureTableDescriptorCount};

		if (capabilityReport != nullptr)
		{
			result.BackendRayTracingAvailable = capabilityReport->Core.SupportsRayTracing;
			result.InlineRayQueryAvailable = capabilityReport->Core.SupportsInlineRayQuery;
			result.DescriptorTlasSupported = capabilityReport->TlasShaderAccess.SupportsDescriptor;
			result.DeviceAddressTlasSupported = capabilityReport->TlasShaderAccess.SupportsShaderDeviceAddress;
		}

		return result;
	}

	bool CanUseSceneTlas(
	    const RayTracingPassCapabilities& capabilities,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		if (!capabilities.BackendRayTracingAvailable || !capabilities.BoundSceneTlasAvailable ||
		    capabilities.SceneTlasShaderAccessMode != accessMode)
		{
			return false;
		}

		switch (accessMode)
		{
			case RayTracingSceneTlasShaderAccessMode::Descriptor:
				return capabilities.DescriptorTlasSupported;
			case RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress:
				return capabilities.DeviceAddressTlasSupported;
		}

		return false;
	}
}
