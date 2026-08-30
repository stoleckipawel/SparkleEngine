#include "PCH.h"

#include "Cooking/ShaderCompileBatch.h"

#include "Cooking/ShaderCompileFailureReplay.h"
#include "Cooking/ShaderCompileJobExecutor.h"
#include "Cooking/ShaderCookCancellation.h"
#include "Cooking/ShaderCookSettings.h"
#include "Cooking/ShaderDebugArtifactWriter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "TaskExecutor.h"
#include "Verification/ShaderParameterStructCookVerifier.h"

#include <algorithm>
#include <exception>
#include <thread>
#include <unordered_map>

std::vector<ShaderCompileResult> ShaderCompileBatch::Execute(const ShaderCookSettings& settings, std::span<const ShaderCompileJob> jobs)
{
	if (jobs.empty())
	{
		return {};
	}
	ShaderCookCancellation::ThrowIfRequested(settings.cancellationSignalPath);

	const ProducerMap producerMap = SelectProducers(jobs);
	const std::vector<ShaderCompileResult> producerResults = CompileProducers(settings, jobs, producerMap.ProducerJobIndices);
	std::vector<ShaderCompileResult> results = FanOutResults(jobs, producerResults, producerMap.ProducerForJob);
	FinalizeResults(settings, jobs, results);
	return results;
}

ShaderCompileBatch::ProducerMap ShaderCompileBatch::SelectProducers(std::span<const ShaderCompileJob> jobs)
{
	ProducerMap result;
	result.ProducerForJob.resize(jobs.size());
	std::unordered_map<ShaderCompileInputHash, std::vector<std::size_t>> producersByHash;
	for (std::size_t jobIndex = 0; jobIndex < jobs.size(); ++jobIndex)
	{
		const ShaderCompileJob& job = jobs[jobIndex];
		std::size_t producerIndex = result.ProducerJobIndices.size();
		if (const auto matchingHash = producersByHash.find(job.InputHash); matchingHash != producersByHash.end())
		{
			for (const std::size_t candidateProducer : matchingHash->second)
			{
				if (HasSameCompilerInput(jobs[result.ProducerJobIndices[candidateProducer]], job))
				{
					producerIndex = candidateProducer;
					break;
				}
			}
		}
		if (producerIndex == result.ProducerJobIndices.size())
		{
			result.ProducerJobIndices.push_back(jobIndex);
			producersByHash[job.InputHash].push_back(producerIndex);
		}
		result.ProducerForJob[jobIndex] = producerIndex;
	}
	return result;
}

std::vector<ShaderCompileResult> ShaderCompileBatch::CompileProducers(
    const ShaderCookSettings& settings,
    std::span<const ShaderCompileJob> jobs,
    std::span<const std::size_t> producerJobIndices)
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	const std::uint32_t compileWorkers = std::clamp(settings.maximumParallelCompiles, 1u, std::min(hardwareThreads, 8u));
	TaskExecutor executor(
	    TaskExecutorConfig{
	        .FrameCriticalWorkerCount = 1,
	        .BackgroundWorkerCount = compileWorkers,
	        .MaximumTasksPerExecution = static_cast<std::uint32_t>(producerJobIndices.size()),
	        .MaximumEdgesPerExecution = 1,
	        .MaximumActiveExecutions = 1});
	std::vector<ShaderCompileResult> results(producerJobIndices.size());
	std::vector<std::string> failureDiagnostics(producerJobIndices.size());
	TaskGraphBuilder graph(TaskGraphLimits{.MaximumTasks = static_cast<std::uint32_t>(producerJobIndices.size()), .MaximumEdges = 1});
	for (std::uint32_t producerIndex = 0; producerIndex < producerJobIndices.size(); ++producerIndex)
	{
		graph.Add(
		    TaskDesc{TaskName("Compile shader"), TaskLane::Background},
		    [&, producerIndex](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested() || ShaderCookCancellation::IsRequested(settings.cancellationSignalPath))
			    {
				    return TaskResult::Cancelled("Shader cook was cancelled.");
			    }
			    const std::size_t jobIndex = producerJobIndices[producerIndex];
			    try
			    {
				    results[producerIndex] = ShaderCompileJobExecutor::Execute(jobs[jobIndex]);
				    return TaskResult::Success();
			    }
			    catch (const std::exception& error)
			    {
				    failureDiagnostics[producerIndex] = error.what();
				    return TaskResult::Failure(error.what());
			    }
		    });
	}

	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(graph.Compile(), context);
	if (execution.GetStatus() == TaskExecutionStatus::Succeeded)
	{
		ShaderCookCancellation::ThrowIfRequested(settings.cancellationSignalPath);
		return results;
	}
	for (std::size_t producerIndex = 0; producerIndex < failureDiagnostics.size(); ++producerIndex)
	{
		if (!failureDiagnostics[producerIndex].empty())
		{
			ShaderCompileFailureReplay::Write(
			    Filesystem::GetCookedShaderRootPath(),
			    jobs[producerJobIndices[producerIndex]],
			    failureDiagnostics[producerIndex]);
			break;
		}
	}

	const TaskResult result = execution.GetResult();
	if (result.GetMessage().empty())
	{
		throw Diagnostics::Error("Shader compile task failed without a diagnostic.");
	}
	throw Diagnostics::Error(std::string(result.GetMessage()));
}

std::vector<ShaderCompileResult> ShaderCompileBatch::FanOutResults(
    std::span<const ShaderCompileJob> jobs,
    std::span<const ShaderCompileResult> producerResults,
    std::span<const std::size_t> producerForJob)
{
	std::vector<ShaderCompileResult> results;
	results.reserve(jobs.size());
	for (std::size_t jobIndex = 0; jobIndex < jobs.size(); ++jobIndex)
	{
		results.push_back(producerResults[producerForJob[jobIndex]]);
		results.back().ShaderType = jobs[jobIndex].Request.ShaderType;
		results.back().Target = jobs[jobIndex].Request.Target;
	}
	return results;
}

void ShaderCompileBatch::FinalizeResults(
    const ShaderCookSettings& settings,
    std::span<const ShaderCompileJob> jobs,
    std::span<ShaderCompileResult> results)
{
	for (std::size_t jobIndex = 0; jobIndex < jobs.size(); ++jobIndex)
	{
		const ShaderCompileJob& job = jobs[jobIndex];
		ShaderCompileResult& result = results[jobIndex];
		try
		{
			ShaderParameterStructCookVerifier::Verify(job, result.Output, &result.DebugArtifacts);
		}
		catch (const std::exception& error)
		{
			ShaderCompileFailureReplay::Write(Filesystem::GetCookedShaderRootPath(), job, error.what());
			throw;
		}
		if (job.Request.CaptureDebugArtifacts)
		{
			ShaderDebugArtifactWriter::Write(settings.debugArtifactDirectory, job.Request, result.Output, result.DebugArtifacts);
		}
	}
}

bool ShaderCompileBatch::HasSameCompilerInput(const ShaderCompileJob& lhs, const ShaderCompileJob& rhs) noexcept
{
	if (lhs.SourceContentHash != rhs.SourceContentHash || lhs.DependencyClosureHash != rhs.DependencyClosureHash
	    || lhs.RequestHash != rhs.RequestHash || lhs.VirtualDependencies != rhs.VirtualDependencies || lhs.BackendName != rhs.BackendName
	    || lhs.BackendVersion != rhs.BackendVersion || lhs.TargetProfile != rhs.TargetProfile
	    || lhs.Request.VirtualSourcePath != rhs.Request.VirtualSourcePath || lhs.Request.SourceCode != rhs.Request.SourceCode
	    || lhs.Request.EntryPoint != rhs.Request.EntryPoint || lhs.Request.Stage != rhs.Request.Stage
	    || lhs.Request.Target != rhs.Request.Target || lhs.Request.UnitKind != rhs.Request.UnitKind
	    || lhs.Request.RequiredFeatures != rhs.Request.RequiredFeatures || lhs.Request.EnableDebugInfo != rhs.Request.EnableDebugInfo
	    || lhs.Request.EnableOptimizations != rhs.Request.EnableOptimizations
	    || lhs.Request.TreatWarningsAsErrors != rhs.Request.TreatWarningsAsErrors
	    || lhs.Request.StripDebugInfo != rhs.Request.StripDebugInfo || lhs.Request.Defines != rhs.Request.Defines
	    || lhs.Request.DescriptorBindingRemaps.size() != rhs.Request.DescriptorBindingRemaps.size())
	{
		return false;
	}
	for (std::size_t index = 0; index < lhs.Request.DescriptorBindingRemaps.size(); ++index)
	{
		const ShaderDescriptorBindingRemap& left = lhs.Request.DescriptorBindingRemaps[index];
		const ShaderDescriptorBindingRemap& right = rhs.Request.DescriptorBindingRemaps[index];
		if (left.Name != right.Name || left.Set != right.Set || left.Binding != right.Binding)
		{
			return false;
		}
	}
	return true;
}
