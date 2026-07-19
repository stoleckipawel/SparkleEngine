#include "PCH.h"

#include "Concurrency/TaskRuntimeCVars.h"

#include "Core/Public/Console/CVar.h"

namespace
{
	ConsoleVariable<std::uint32_t> g_workerCount(
	    "task.WorkerCount", 0, "Background task-worker override; 0 selects the conservative one-worker default.");
	ConsoleVariable<bool> g_serialExecution(
	    "task.SerialExecution", false, "Run SparkleTasks on the deterministic caller-thread reference executor.");
}

namespace TaskRuntimeCVars
{
	void Register() noexcept {}
	std::uint32_t ResolveWorkerCount() noexcept { return g_workerCount.Get(); }
	bool UseSerialExecution() noexcept { return g_serialExecution.Get(); }
}
