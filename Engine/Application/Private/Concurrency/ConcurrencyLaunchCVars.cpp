#include "PCH.h"

#include "Concurrency/ConcurrencyLaunchCVars.h"

#include "Core/Public/Console/CVar.h"

#include <cstdint>

namespace
{
	ConsoleVariable<std::uint32_t> g_taskWorkerCount(
	    "task.WorkerCount", 0, "Development override for future engine task workers; 0 selects the measured default.");
	ConsoleVariable<bool> g_taskSerialExecution(
	    "task.SerialExecution", true, "Development serial oracle for the future task executor.");
	ConsoleVariable<bool> g_threadedRenderer(
	    "r.ThreadedRenderer", false, "Development switch reserved for the future renderer owner thread.");
	ConsoleVariable<bool> g_parallelCommandRecording(
	    "r.ParallelCommandRecording", false, "Development switch reserved for future parallel RHI command recording.");
	ConsoleVariable<std::uint32_t> g_renderPipelineDepth(
	    "r.RenderPipelineDepth", 0, "Development renderer pipeline depth; 0 is synchronous and 1 is the future bounded one-ahead mode.");
}

namespace ConcurrencyLaunchCVars
{
	void Register() noexcept {}
}
