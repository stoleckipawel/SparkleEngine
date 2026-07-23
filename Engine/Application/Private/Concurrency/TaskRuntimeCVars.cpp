#include "PCH.h"

#include "Concurrency/TaskRuntimeCVars.h"

#include "Core/Public/Console/CVar.h"

class TaskRuntimeCVarsState final
{
  public:
	inline static ConsoleVariable<std::uint32_t> g_workerCount{
	    "task.WorkerCount", 0, "Frame-critical/background task-worker override; 0 selects one worker per CPU lane."};
	inline static ConsoleVariable<bool> g_serialExecution{
	    "task.SerialExecution", false, "Run SparkleTasks on the deterministic caller-thread reference executor."};
};

namespace TaskRuntimeCVars
{
	void Register() noexcept {}
	std::uint32_t ResolveWorkerCount() noexcept { return TaskRuntimeCVarsState::g_workerCount.Get(); }
	bool UseSerialExecution() noexcept { return TaskRuntimeCVarsState::g_serialExecution.Get(); }
}
