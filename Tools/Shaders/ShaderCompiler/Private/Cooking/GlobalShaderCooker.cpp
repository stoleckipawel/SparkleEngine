#include "PCH.h"

#include "Cooking/GlobalShaderCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderArtifactPublication.h"
#include "Cooking/ShaderCompileBatch.h"
#include "Cooking/ShaderCookCancellation.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"

#include <unordered_set>

ShaderCookResult GlobalShaderCooker::CookAll(const ShaderCookSettings& settings, const ToolWorkProgressCallback& progress) const
{
	ShaderCookResult result;
	result.outputDirectory = Filesystem::GetCookedShaderRootPath();
	if (progress)
	{
		progress({.Action = "Planning shader cook"});
	}
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan = ShaderCookPlanBuilder::Build(settings, backendPool, progress);
	result.compileJobCount = plan.jobs.size();
	std::unordered_set<ShaderTypeId> selectedShaderTypes;
	for (const ShaderCompileJob& job : plan.jobs)
	{
		selectedShaderTypes.insert(job.Request.ShaderType);
	}
	result.selectedShaderCount = selectedShaderTypes.size();
	if (plan.jobs.empty() && plan.removedShaderTypeCount == 0)
	{
		if (progress)
		{
			progress({.Action = "Shader cook is up to date", .Completed = 1, .Total = 1});
		}
		return result;
	}
	const std::size_t totalWork = plan.jobs.size() * 2 + 1;
	if (progress)
	{
		progress({.Action = plan.jobs.empty() ? "Preparing shader publication" : "Compiling shaders", .Total = totalWork});
	}

	std::vector<ShaderCompileResult> compileResults;
	if (!plan.jobs.empty())
	{
		compileResults = ShaderCompileBatch::Execute(settings, plan.jobs, totalWork, progress);
	}
	if (progress)
	{
		progress({.Action = "Preparing shader publication", .Completed = totalWork - 1, .Total = totalWork});
	}
	for (const ShaderCompileConsumer& consumer : plan.consumers)
	{
		const ShaderCompileJob& job = plan.jobs[consumer.JobIndex];
		const ShaderCookDesc& shader = plan.shaders[consumer.ShaderIndex];
		ShaderCompileResult& compiled = compileResults[consumer.JobIndex];
		if (compiled.ShaderType != job.Request.ShaderType || compiled.Target != job.Request.Target || compiled.InputHash != job.InputHash)
		{
			throw Diagnostics::Error("Shader compile result does not match its immutable job identity.");
		}
		plan.shaderOutputs[consumer.ShaderIndex].push_back(
		    ShaderCookProduct{
		        .shaderTypeId = shader.shaderTypeId,
		        .target = compiled.Target,
		        .features = shader.features,
		        .rayTracing = shader.rayTracing,
		        .parameterLayout = shader.parameterLayout,
		        .bindingRemaps = job.Request.DescriptorBindingRemaps,
		        .compiled = std::move(compiled.Output)});
	}

	ShaderCookCancellation::ThrowIfRequested(settings.cancellationSignalPath);
	if (progress)
	{
		progress({.Action = "Publishing cooked shaders", .Completed = totalWork - 1, .Total = totalWork});
	}
	const bool replaceCompleteCatalog = settings.shaderId.empty() && settings.changedVirtualPaths.empty();
	result.output = ShaderArtifactPublication::Publish(plan, result.outputDirectory, replaceCompleteCatalog);
	if (progress)
	{
		progress({.Action = "Cooked shaders published", .Completed = totalWork, .Total = totalWork});
	}
	return result;
}
