#include "../PCH.h"
#include "Textures/CookedTextureLoadTask.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskGraph.h"
#include "Tasks/Public/TaskScope.h"

static const auto g_cookedTextureLoadTaskLogger = Logging::GetOrCreateLogger("Renderer.CookedTextureLoadTask");

TaskExecution CookedTextureLoadTask::Launch(
    TaskExecutor& taskExecutor,
    TaskScope& taskScope,
    const std::filesystem::path& path,
    const std::shared_ptr<Payload>& payload)
{
	if (payload == nullptr)
	{
		Diagnostics::Fatal(g_cookedTextureLoadTaskLogger, __FILE__, __LINE__, "Texture loading task requires an output payload.");
	}

	TaskGraphBuilder graph;
	const TaskNodeHandle read = graph.Add(
	    TaskDesc{.Name = TaskName("Read cooked texture generation"), .Lane = TaskLane::BlockingIo},
	    [path, payload](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    return TaskResult::Cancelled("Texture read cancelled.");
		    }
		    payload->File = CookedTextureLoader::Read(path);
		    return TaskResult::Success();
	    });
	graph.ContinueWith(
	    read,
	    TaskDesc{.Name = TaskName("Decode cooked texture generation"), .Lane = TaskLane::Background},
	    [payload](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    payload->File = {};
			    return TaskResult::Cancelled("Texture decode cancelled.");
		    }
		    payload->Texture = CookedTextureLoader::Decode(payload->File);
		    payload->File = {};
		    return TaskResult::Success();
	    });

	TaskExecution execution = taskExecutor.Launch(taskScope, graph.Compile());
	if (!execution.IsValid())
	{
		Diagnostics::Fatal(g_cookedTextureLoadTaskLogger, __FILE__, __LINE__, "Texture loading task graph launch failed.");
	}
	return execution;
}
