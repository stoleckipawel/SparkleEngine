#pragma once

#include <cstdint>

namespace TaskRuntimeCVars
{
	void Register() noexcept;
	std::uint32_t ResolveFrameCriticalWorkerCount() noexcept;
	std::uint32_t ResolveBackgroundWorkerCount() noexcept;
	std::uint32_t ResolveBlockingIoWorkerCount() noexcept;
	bool UseSerialExecution() noexcept;
}
