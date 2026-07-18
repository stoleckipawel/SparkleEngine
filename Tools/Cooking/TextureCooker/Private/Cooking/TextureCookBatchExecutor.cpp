#include "PCH.h"

#include "Cooking/TextureCookBatchExecutor.h"

#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookMemoryLimiter.h"
#include "Core/Public/Files/FileUtils.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <objbase.h>
#include <thread>

namespace
{
	class ScopedComInitializer final
	{
	  public:
		~ScopedComInitializer()
		{
			if (SUCCEEDED(m_result))
				CoUninitialize();
		}

		bool TryInitialize(std::string& outErrorMessage)
		{
			m_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			if (FAILED(m_result) && m_result != RPC_E_CHANGED_MODE)
			{
				outErrorMessage = "failed to initialize COM for source texture loading";
				return false;
			}
			outErrorMessage.clear();
			return true;
		}

	  private:
		HRESULT m_result = E_FAIL;
	};

	std::filesystem::path BuildStagedOutputPath(const std::filesystem::path& outputPath)
	{
		return Files::BuildTemporaryPath(outputPath, ".cook-generation");
	}
}

TextureCookBatchExecutionResult TextureCookBatchExecutor::Execute(
    const std::vector<TextureCookRequest>& requests,
    std::size_t memoryBudgetBytes)
{
	TextureCookBatchExecutionResult batchResult;
	batchResult.Items.resize(requests.size());
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	const std::uint32_t backgroundWorkers = std::clamp(hardwareThreads > 1 ? hardwareThreads - 1 : 1u, 1u, 4u);
	TaskExecutor executor(
	    TaskExecutorConfig{
	        .FrameCriticalWorkerCount = 1,
	        .BackgroundWorkerCount = backgroundWorkers,
	        .MaximumTasksPerExecution = static_cast<std::uint32_t>(std::max<std::size_t>(requests.size(), 1)),
	        .MaximumEdgesPerExecution = 1,
	        .MaximumActiveExecutions = 1});
	TextureCookMemoryLimiter memoryLimiter(memoryBudgetBytes);
	TaskGraphBuilder graph(
	    TaskGraphLimits{.MaximumTasks = static_cast<std::uint32_t>(std::max<std::size_t>(requests.size(), 1)), .MaximumEdges = 1});
	for (std::uint32_t index = 0; index < requests.size(); ++index)
	{
		graph.Add(
		    TaskDesc{TaskName("Cook texture request"), TaskLane::Background},
		    [&, index](TaskExecutionContext& context)
		    {
			    TextureCookBatchItemResult& itemResult = batchResult.Items[index];
			    TextureCookRequest stagedRequest = requests[index];
			    itemResult.StagedOutputPath = BuildStagedOutputPath(stagedRequest.outputPath);
			    stagedRequest.outputPath = itemResult.StagedOutputPath;
			    Files::CleanupTemporaryFile(itemResult.StagedOutputPath);
			    ScopedComInitializer com;
			    if (!com.TryInitialize(itemResult.Diagnostic))
				    return TaskResult::Failure(itemResult.Diagnostic);
			    TextureAssetCooker cooker;
			    itemResult.Succeeded = cooker.Cook(stagedRequest, memoryLimiter, context.GetCancellationToken(), itemResult.Diagnostic);
			    return itemResult.Succeeded ? TaskResult::Success() : TaskResult::Failure(itemResult.Diagnostic);
		    });
	}
	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(graph.Compile(), context);
	batchResult.Succeeded = execution.GetStatus() == TaskExecutionStatus::Succeeded;
	batchResult.PeakAdmittedBytes = memoryLimiter.GetPeakBytes();
	return batchResult;
}
