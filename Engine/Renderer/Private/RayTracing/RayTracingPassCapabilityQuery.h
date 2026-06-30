#pragma once

#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"

#include <cstdint>

struct FrameContext;
struct RenderRayTracingPassServices;

struct RayTracingPassCapabilities final
{
	bool BackendRayTracingAvailable = false;
	bool InlineRayQueryAvailable = false;
	bool BoundSceneTlasAvailable = false;
	RayTracingSceneTlasShaderAccessMode SceneTlasShaderAccessMode = RayTracingSceneTlasShaderAccessMode::Descriptor;
	bool DescriptorTlasSupported = false;
	bool DeviceAddressTlasSupported = false;
	bool TriangleMaterialDataAvailable = false;
	bool HitDataAvailable = false;
	bool MaterialTextureTableAvailable = false;
	std::uint32_t MaterialTextureTableDescriptorCount = 0u;
};

namespace RayTracingPassCapabilityQuery
{
	RayTracingPassCapabilities Build(
	    const FrameContext& frame,
	    const RenderRayTracingPassServices* rayTracingServices) noexcept;
	bool CanUseSceneTlas(
	    const RayTracingPassCapabilities& capabilities,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept;
}
