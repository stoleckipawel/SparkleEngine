#pragma once

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

struct RayTracingSceneTlasAddressBinding final
{
	RhiGpuVirtualAddress GpuAddress = 0u;
	bool IsAvailable = false;
};

namespace RayTracingScenePassBinding
{
	inline bool CanUseSceneTlas(
	    const RayTracingPassCapabilities& capabilities,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, accessMode);
	}

	inline bool FrameUsesSceneTlasAccessMode(
	    const FrameContext& frame,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		return frame.rayTracingScene.HasBoundTlas() && frame.rayTracingScene.TlasShaderAccessMode == accessMode;
	}

	template <typename TParameterInstance>
	bool BindSceneTlas(
	    FrameGraphBuilder& builder,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    RayTracingSceneTlasShaderAccessMode accessMode,
	    TParameterInstance& parameters)
	{
		if (accessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
		{
			return false;
		}

		parameters->SceneTlas = builder.Read(sceneTlas);
		return true;
	}

	inline RayTracingSceneTlasAddressBinding BindSceneTlasAddress(
	    const RenderRayTracingPassServices* services,
	    RayTracingSceneTlasShaderAccessMode accessMode) noexcept
	{
		if (accessMode != RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress || services == nullptr ||
		    services->Scene == nullptr)
		{
			return {};
		}

		const RhiGpuVirtualAddress gpuAddress = services->Scene->GetTlasGpuAddress();
		return RayTracingSceneTlasAddressBinding{.GpuAddress = gpuAddress, .IsAvailable = gpuAddress != 0u};
	}

	inline std::uint32_t GetGpuAddressLow(RhiGpuVirtualAddress gpuAddress) noexcept
	{
		return static_cast<std::uint32_t>(gpuAddress & 0xFFFFFFFFull);
	}

	inline std::uint32_t GetGpuAddressHigh(RhiGpuVirtualAddress gpuAddress) noexcept
	{
		return static_cast<std::uint32_t>((gpuAddress >> 32u) & 0xFFFFFFFFull);
	}
}
