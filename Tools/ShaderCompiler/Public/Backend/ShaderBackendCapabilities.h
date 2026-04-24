#pragma once

#include "Backend/ShaderTarget.h"

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

	bool SupportsDxil = false;
	bool SupportsSpirV = false;
};
