#pragma once

#include <cstdint>

namespace ConcurrencyLaunchCVars
{
	void Register() noexcept;
	bool UseThreadedRenderer() noexcept;
	std::uint32_t ResolveRenderPipelineDepth() noexcept;
}
