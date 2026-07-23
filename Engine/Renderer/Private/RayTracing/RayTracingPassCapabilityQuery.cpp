#include "../PCH.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"

#include "Frame/Core/FrameContext.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"

class RayTracingPassCapabilityQueryOperations final
{
  public:
	static const RayTracingCapabilityReport* ResolveCapabilityReport(const RenderRayTracingPassServices* rayTracingServices) noexcept
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

};

namespace RayTracingPassCapabilityQuery
{
	RayTracingPassCapabilities Build(
	    const FrameContext& frame,
	    const RenderRayTracingPassServices* rayTracingServices) noexcept
	{
		const RayTracingCapabilityReport* capabilityReport = RayTracingPassCapabilityQueryOperations::ResolveCapabilityReport(rayTracingServices);
		RayTracingPassCapabilities result{
		    .BoundSceneTlasAvailable = frame.rayTracingScene.HasBoundTlas(),
		    .SceneTlasShaderAccessMode = frame.rayTracingScene.TlasShaderAccessMode,
		    .TriangleMaterialDataAvailable = frame.sceneGpuData.RayTracing.IsValid(),
		    .HitDataAvailable = frame.sceneGpuData.RayTracing.IsValid() && frame.sceneGpuData.Geometry.HasMeshInstances(),
		    .MaterialTextureTableAvailable = static_cast<bool>(frame.sceneData.materialTextureTable)};

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
