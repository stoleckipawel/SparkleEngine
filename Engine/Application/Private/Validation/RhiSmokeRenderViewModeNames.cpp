#include "PCH.h"

#include "Validation/RhiSmokeRenderViewModeNames.h"

#include <algorithm>
#include <cctype>

namespace RhiSmokeRenderViewModeNames
{
	bool EqualsName(std::string_view lhs, std::string_view rhs) noexcept
	{
		return lhs.size() == rhs.size() && std::equal(
		                                      lhs.begin(),
		                                      lhs.end(),
		                                      rhs.begin(),
		                                      [](char a, char b)
		                                      {
			                                      return std::tolower(static_cast<unsigned char>(a)) ==
			                                             std::tolower(static_cast<unsigned char>(b));
		                                      });
	}

	const char* ToString(RenderViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case RenderViewMode::Lit:
				return "Lit";
			case RenderViewMode::Wireframe:
				return "Wireframe";
			case RenderViewMode::GBufferDiffuse:
				return "GBufferDiffuse";
			case RenderViewMode::GBufferNormal:
				return "GBufferNormal";
			case RenderViewMode::GBufferRoughness:
				return "GBufferRoughness";
			case RenderViewMode::GBufferMetallic:
				return "GBufferMetallic";
			case RenderViewMode::GBufferEmissive:
				return "GBufferEmissive";
			case RenderViewMode::GBufferAmbientOcclusion:
				return "GBufferAmbientOcclusion";
			case RenderViewMode::GBufferSubsurfaceColor:
				return "GBufferSubsurfaceColor";
			case RenderViewMode::GBufferSubsurfaceStrength:
				return "GBufferSubsurfaceStrength";
			case RenderViewMode::DirectDiffuse:
				return "DirectDiffuse";
			case RenderViewMode::DirectSpecular:
				return "DirectSpecular";
			case RenderViewMode::DirectSubsurface:
				return "DirectSubsurface";
			case RenderViewMode::IndirectDiffuse:
				return "IndirectDiffuse";
			case RenderViewMode::IndirectSpecular:
				return "IndirectSpecular";
			case RenderViewMode::IndirectSubsurface:
				return "IndirectSubsurface";
			case RenderViewMode::InstanceGroups:
				return "InstanceGroups";
			case RenderViewMode::RayTracingPartitions:
				return "RayTracingPartitions";
			case RenderViewMode::RayTracingPartitionUpdates:
				return "RayTracingPartitionUpdates";
			case RenderViewMode::RayTracingInstanceMovement:
				return "RayTracingInstanceMovement";
			case RenderViewMode::RayTracingGpuDrivenUpdates:
				return "RayTracingGpuDrivenUpdates";
			case RenderViewMode::RayTracingTopLevelMode:
				return "RayTracingTopLevelMode";
			case RenderViewMode::RayTracingNativeOperations:
				return "RayTracingNativeOperations";
			case RenderViewMode::RayTracingProviderStatus:
				return "RayTracingProviderStatus";
			case RenderViewMode::Count:
				break;
		}

		return "Unknown";
	}

	bool TryParseAlias(std::string_view value, RenderViewMode& outViewMode) noexcept
	{
		if (EqualsName(value, "Partitions"))
		{
			outViewMode = RenderViewMode::RayTracingPartitions;
			return true;
		}
		if (EqualsName(value, "PartitionUpdates"))
		{
			outViewMode = RenderViewMode::RayTracingPartitionUpdates;
			return true;
		}
		if (EqualsName(value, "TopLevelMode"))
		{
			outViewMode = RenderViewMode::RayTracingTopLevelMode;
			return true;
		}
		if (EqualsName(value, "NativeOperations"))
		{
			outViewMode = RenderViewMode::RayTracingNativeOperations;
			return true;
		}
		if (EqualsName(value, "GpuDrivenUpdates"))
		{
			outViewMode = RenderViewMode::RayTracingGpuDrivenUpdates;
			return true;
		}
		if (EqualsName(value, "ProviderStatus"))
		{
			outViewMode = RenderViewMode::RayTracingProviderStatus;
			return true;
		}
		return false;
	}

	bool TryParse(std::string_view value, RenderViewMode& outViewMode) noexcept
	{
		for (std::uint32_t viewModeIndex = 0; viewModeIndex < static_cast<std::uint32_t>(RenderViewMode::Count); ++viewModeIndex)
		{
			const RenderViewMode viewMode = static_cast<RenderViewMode>(viewModeIndex);
			if (EqualsName(value, ToString(viewMode)))
			{
				outViewMode = viewMode;
				return true;
			}
		}
		return TryParseAlias(value, outViewMode);
	}
}
