#include "PCH.h"

#include "Cooking/TextureCookBatchExecutor.h"

#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookMemoryLimiter.h"
#include "Core/Public/Files/FileUtils.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <objbase.h>
#include <thread>
#include <utility>

class TextureSourceComApartment final
{
  public:
	~TextureSourceComApartment();

	bool TryInitialize(std::string& outErrorMessage);

  private:
	HRESULT m_result = E_FAIL;
};

class TextureCookBatchRun final
{
  public:
	TextureCookBatchRun(const std::vector<TextureCookRequest>& requests, std::size_t memoryBudgetBytes);

	TextureCookBatchExecutionResult Execute();

  private:
	TaskExecutorConfig BuildExecutorConfig() const;
	CompiledTaskGraph BuildTaskGraph();
	TaskResult CookRequest(std::uint32_t index, TaskExecutionContext& context);
	static std::uint32_t ResolveBackgroundWorkerCount();
	static std::filesystem::path BuildStagedOutputPath(const std::filesystem::path& outputPath);

	const std::vector<TextureCookRequest>& m_requests;
	TextureCookMemoryLimiter m_memoryLimiter;
	TextureCookBatchExecutionResult m_result;
};

TextureSourceComApartment::~TextureSourceComApartment()
{
	if (SUCCEEDED(m_result))
	{
		CoUninitialize();
	}
}

bool TextureSourceComApartment::TryInitialize(std::string& outErrorMessage)
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

TextureCookBatchRun::TextureCookBatchRun(
    const std::vector<TextureCookRequest>& requests,
    std::size_t memoryBudgetBytes) :
    m_requests(requests),
    m_memoryLimiter(memoryBudgetBytes)
{
}

TextureCookBatchExecutionResult TextureCookBatchRun::Execute()
{
	m_result.Items.resize(m_requests.size());

	TaskExecutor executor(BuildExecutorConfig());
	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(BuildTaskGraph(), context);

	m_result.Succeeded = execution.GetStatus() == TaskExecutionStatus::Succeeded;
	return std::move(m_result);
}

TaskExecutorConfig TextureCookBatchRun::BuildExecutorConfig() const
{
	const std::uint32_t maximumTasks =
	    static_cast<std::uint32_t>(std::max<std::size_t>(m_requests.size(), 1));
	return TaskExecutorConfig{
	    .FrameCriticalWorkerCount = 1,
	    .BackgroundWorkerCount = ResolveBackgroundWorkerCount(),
	    .MaximumTasksPerExecution = maximumTasks,
	    .MaximumEdgesPerExecution = 1,
	    .MaximumActiveExecutions = 1};
}

CompiledTaskGraph TextureCookBatchRun::BuildTaskGraph()
{
	const std::uint32_t maximumTasks =
	    static_cast<std::uint32_t>(std::max<std::size_t>(m_requests.size(), 1));
	TaskGraphBuilder graph(
	    TaskGraphLimits{.MaximumTasks = maximumTasks, .MaximumEdges = 1});

	for (std::uint32_t index = 0; index < m_requests.size(); ++index)
	{
		graph.Add(
		    TaskDesc{TaskName("Cook texture request"), TaskLane::Background},
		    [this, index](TaskExecutionContext& context)
		    {
			    return CookRequest(index, context);
		    });
	}

	return graph.Compile();
}

TaskResult TextureCookBatchRun::CookRequest(
    std::uint32_t index,
    TaskExecutionContext& context)
{
	TextureCookBatchItemResult& itemResult = m_result.Items[index];
	TextureCookRequest stagedRequest = m_requests[index];
	itemResult.StagedOutputPath = BuildStagedOutputPath(stagedRequest.outputPath);
	stagedRequest.outputPath = itemResult.StagedOutputPath;
	Files::CleanupTemporaryFile(itemResult.StagedOutputPath);

	TextureSourceComApartment comApartment;
	if (!comApartment.TryInitialize(itemResult.Diagnostic))
	{
		return TaskResult::Failure(itemResult.Diagnostic);
	}

	TextureAssetCooker cooker;
	itemResult.Succeeded =
	    cooker.Cook(stagedRequest, m_memoryLimiter, context.GetCancellationToken(), itemResult.Diagnostic);
	return itemResult.Succeeded ? TaskResult::Success() : TaskResult::Failure(itemResult.Diagnostic);
}

std::uint32_t TextureCookBatchRun::ResolveBackgroundWorkerCount()
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	return std::clamp(hardwareThreads > 1 ? hardwareThreads - 1 : 1u, 1u, 4u);
}

std::filesystem::path TextureCookBatchRun::BuildStagedOutputPath(
    const std::filesystem::path& outputPath)
{
	return Files::BuildTemporaryPath(outputPath, ".cook-generation");
}

TextureCookBatchExecutionResult TextureCookBatchExecutor::Execute(
    const std::vector<TextureCookRequest>& requests,
    std::size_t memoryBudgetBytes)
{
	return TextureCookBatchRun(requests, memoryBudgetBytes).Execute();
}
