#pragma once

#include <cstdint>

namespace TaskRuntimeCVars
{
	void Register() noexcept;
	std::uint32_t ResolveWorkerCount() noexcept;
	bool UseSerialExecution() noexcept;
}
