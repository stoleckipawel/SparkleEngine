#pragma once

#include "RHI/Public/Shaders/ShaderTarget.h"

// What a backend can produce.
// The orchestrator picks the first backend that supports the requested target.
struct ShaderBackendCapabilities final
{
	bool SupportsTarget(ShaderTarget target) const noexcept
	{
		if (IsDxilTarget(target))
		{
			return SupportsDxil;
		}
		if (IsSpirVTarget(target))
		{
			return SupportsSpirV;
		}
		return false;
	}

	bool SupportsRayTracingLibrary(ShaderTarget target) const noexcept
	{
		if (IsDxilTarget(target))
		{
			return SupportsDxilRayTracingLibrary;
		}
		if (IsSpirVTarget(target))
		{
			return SupportsSpirVRayTracingLibrary;
		}
		return false;
	}

	bool SupportsInlineRayQuery(ShaderTarget target) const noexcept
	{
		if (IsDxilTarget(target))
		{
			return SupportsDxilInlineRayQuery;
		}
		if (IsSpirVTarget(target))
		{
			return SupportsSpirVInlineRayQuery;
		}
		return false;
	}

	bool SupportsDxil = false;
	bool SupportsSpirV = false;
	bool SupportsDxilRayTracingLibrary = false;
	bool SupportsSpirVRayTracingLibrary = false;
	bool SupportsDxilInlineRayQuery = false;
	bool SupportsSpirVInlineRayQuery = false;
};
