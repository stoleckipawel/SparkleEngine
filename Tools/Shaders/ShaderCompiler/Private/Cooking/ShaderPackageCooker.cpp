#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendPool.h"
#include "Cooking/CookedShaderPackageEmitter.h"
#include "Cooking/ShaderCompileBatch.h"
#include "Cooking/ShaderCookCancellation.h"
#include "Cooking/ShaderCookPlanBuilder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"
#include <unordered_set>
#include <vector>

ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	ShaderPackageCookResult result;
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
		CookedStageBuild output = std::move(compileResults[consumer.JobIndex].Output);
		const ShaderCookPackageDesc& package = plan.packages[consumer.PackageIndex];
		if (output.shaderBlobId == 0)
		{
			output.shaderBlobId =
			    BuildShaderBlobId(package.packageId, output.entryPoint, {}, output.backendName, output.codegenTarget, output.format);
		}
		plan.packageContexts[consumer.PackageIndex].push_back(std::move(output));
	}

	ShaderCookCancellation::ThrowIfRequested(settings.cancellationSignalPath);
	result.packages = CookedShaderPackageEmitter::Emit(plan, result.outputDirectory);

	return result;
}
