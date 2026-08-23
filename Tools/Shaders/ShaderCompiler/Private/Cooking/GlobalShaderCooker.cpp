#include "PCH.h"

#include "Cooking/GlobalShaderCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/ShaderArtifactPublication.h"
#include "Cooking/ShaderCompileBatch.h"
#include "Cooking/ShaderCookCancellation.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Core/Public/FileSystemUtils.h"

#include <unordered_set>

ShaderCookResult GlobalShaderCooker::CookAll(const ShaderCookSettings& settings) const
{
	ShaderCookResult result;
	result.outputDirectory = Filesystem::GetCookedShaderRootPath();
	ShaderBackendPool backendPool;
	ShaderCookPipelinePlan plan = ShaderCookPlanBuilder::Build(settings, backendPool);
	result.compileJobCount = plan.jobs.size();
	std::unordered_set<ShaderTypeId> selectedShaderTypes;
	for (const ShaderCompileJob& job : plan.jobs)
	{
		selectedShaderTypes.insert(job.Request.ShaderType);
	}
	result.selectedShaderCount = selectedShaderTypes.size();
	if (plan.jobs.empty())
	{
		return result;
	}

	std::vector<ShaderCompileResult> compileResults = ShaderCompileBatch::Execute(settings, plan.jobs);
	for (const ShaderCompileConsumer& consumer : plan.consumers)
	{
		const ShaderCompileJob& job = plan.jobs[consumer.JobIndex];
		const ShaderCookDesc& shader = plan.shaders[consumer.ShaderIndex];
		ShaderCompileResult& compiled = compileResults[consumer.JobIndex];
		plan.shaderOutputs[consumer.ShaderIndex].push_back(
		    ShaderCookProduct{
		        .shaderTypeId = shader.shaderTypeId,
		        .target = compiled.Target,
		        .features = shader.features,
		        .parameterLayout = shader.parameterLayout,
		        .bindingRemaps = job.Request.DescriptorBindingRemaps,
		        .compiled = std::move(compiled.Output)});
	}

	ShaderCookCancellation::ThrowIfRequested(settings.cancellationSignalPath);
	const bool completeCatalog = settings.shaderId.empty() && settings.changedVirtualPaths.empty();
	result.output = ShaderArtifactPublication::Publish(plan, result.outputDirectory, completeCatalog);
	return result;
}
