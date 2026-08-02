#include "PCH.h"

#include "Cooking/TextureCookBatchExecutor.h"

#include "Cooking/TextureAssetCooker.h"
#include "Cooking/TextureCookMemoryLimiter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Files/FileUtils.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <objbase.h>
#include <thread>
#include <utility>

static const auto g_textureCookBatchExecutorLogger = Logging::GetOrCreateLogger("TextureCooker.BatchExecutor");

class TextureSourceComApartment final
{
public:
	~TextureSourceComApartment();

	void Initialize();

private:
	HRESULT m_result = E_FAIL;
};

class TextureCookBatchRun final
{
public:
	TextureCookBatchRun(const std::vector<TextureCookRequest>& requests, std::size_t memoryBudgetBytes);

	std::vector<TextureCookBatchItemResult> Execute();

private:
	TaskExecutorConfig BuildExecutorConfig() const;
	CompiledTaskGraph BuildTaskGraph(std::vector<TaskNodeHandle>& outTaskHandles);
	TaskResult CookRequest(std::uint32_t index);
	static std::uint32_t ResolveBackgroundWorkerCount();
	static std::filesystem::path BuildStagedOutputPath(const std::filesystem::path& outputPath);

	const std::vector<TextureCookRequest>& m_requests;
	TextureCookMemoryLimiter m_memoryLimiter;
	std::vector<TextureCookBatchItemResult> m_items;
};

TextureSourceComApartment::~TextureSourceComApartment()
{
	if (SUCCEEDED(m_result))
	{
		CoUninitialize();
	}
}

void TextureSourceComApartment::Initialize()
{
	m_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(m_result) && m_result != RPC_E_CHANGED_MODE)
	{
		throw Diagnostics::Error("Failed to initialize COM for source texture loading.");
	}
}

TextureCookBatchRun::TextureCookBatchRun(const std::vector<TextureCookRequest>& requests, std::size_t memoryBudgetBytes) :
    m_requests(requests),
    m_memoryLimiter(memoryBudgetBytes)
{
}

std::vector<TextureCookBatchItemResult> TextureCookBatchRun::Execute()
{
	m_items.resize(m_requests.size());
	for (std::size_t index = 0; index < m_requests.size(); ++index)
	{
		m_items[index].StagedOutputPath = BuildStagedOutputPath(m_requests[index].outputPath);
	}

	TaskExecutor executor(BuildExecutorConfig());
	TaskExecutionContext context;
	std::vector<TaskNodeHandle> taskHandles;
	const TaskExecution execution = executor.Submit(BuildTaskGraph(taskHandles), context);

	for (std::size_t index = 0; index < taskHandles.size(); ++index)
	{
		const std::optional<TaskResult> taskResult = execution.GetTaskResult(taskHandles[index]);
		if (!taskResult)
		{
			Diagnostics::Fatal(
			    g_textureCookBatchExecutorLogger,
			    __FILE__,
			    __LINE__,
			    "Texture cook execution settled without a result for one of its tasks.");
		}
		m_items[index].CookResult = *taskResult;
	}

	return std::move(m_items);
}

TaskExecutorConfig TextureCookBatchRun::BuildExecutorConfig() const
{
	const std::uint32_t maximumTasks = static_cast<std::uint32_t>(std::max<std::size_t>(m_requests.size(), 1));
	return TaskExecutorConfig{
	    .FrameCriticalWorkerCount = 1,
	    .BackgroundWorkerCount = ResolveBackgroundWorkerCount(),
	    .MaximumTasksPerExecution = maximumTasks,
	    .MaximumEdgesPerExecution = 1,
	    .MaximumActiveExecutions = 1};
}

CompiledTaskGraph TextureCookBatchRun::BuildTaskGraph(std::vector<TaskNodeHandle>& outTaskHandles)
{
	const std::uint32_t maximumTasks = static_cast<std::uint32_t>(std::max<std::size_t>(m_requests.size(), 1));
	TaskGraphBuilder graph(TaskGraphLimits{.MaximumTasks = maximumTasks, .MaximumEdges = 1});
	outTaskHandles.clear();
	outTaskHandles.reserve(m_requests.size());

	for (std::uint32_t index = 0; index < m_requests.size(); ++index)
	{
		outTaskHandles.push_back(graph.Add(
		    TaskDesc{TaskName("Cook texture request"), TaskLane::Background},
		    [this, index](TaskExecutionContext&) { return CookRequest(index); }));
	}

	return graph.Compile();
}

TaskResult TextureCookBatchRun::CookRequest(std::uint32_t index)
{
	TextureCookRequest stagedRequest = m_requests[index];
	stagedRequest.outputPath = m_items[index].StagedOutputPath;
	Files::CleanupTemporaryFile(stagedRequest.outputPath);

	TextureSourceComApartment comApartment;
	comApartment.Initialize();

	TextureAssetCooker cooker;
	cooker.Cook(stagedRequest, m_memoryLimiter);
	return TaskResult::Success();
}

std::uint32_t TextureCookBatchRun::ResolveBackgroundWorkerCount()
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	return std::clamp(hardwareThreads > 1 ? hardwareThreads - 1 : 1u, 1u, 4u);
}

std::filesystem::path TextureCookBatchRun::BuildStagedOutputPath(const std::filesystem::path& outputPath)
{
	return Files::BuildTemporaryPath(outputPath, ".cook-generation");
}

std::vector<TextureCookBatchItemResult> TextureCookBatchExecutor::Execute(
    const std::vector<TextureCookRequest>& requests,
    std::size_t memoryBudgetBytes)
{
	return TextureCookBatchRun(requests, memoryBudgetBytes).Execute();
}
