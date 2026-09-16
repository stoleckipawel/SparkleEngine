#include "PCH.h"

#include "RayTracing/Effects/RayTracingExecutionFrontend.h"

#include "RayTracing/RayTracingCapabilityReport.h"

RayTracingExecutionFrontend ResolveRayTracingExecutionFrontend(const RayTracingCapabilityReport& capabilities) noexcept
{
	const bool sharedRequirements = capabilities.SupportsAccelerationStructure && capabilities.SupportsDescriptorIndexing;
	if (sharedRequirements && capabilities.SupportsRayTracingPipeline)
	{
		return RayTracingExecutionFrontend::Pipeline;
	}
	if (sharedRequirements && capabilities.SupportsInlineRayQuery)
	{
		return RayTracingExecutionFrontend::Inline;
	}
	return RayTracingExecutionFrontend::None;
}
