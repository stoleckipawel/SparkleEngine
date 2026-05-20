#include "PCH.h"

#include "Cooking/CookedStageBuildFinalizer.h"

void CookedStageBuildFinalizer::ApplyNodeMetadata(
    const CookNode& node,
    std::string_view cacheStatus,
    CookedStageBuild& compiledStage)
{
	if (compiledStage.codegenTarget.empty())
	{
		compiledStage.codegenTarget.assign(GetShaderTargetName(node.compileOptions.Target));
	}
	if (compiledStage.shaderBlobId == 0)
	{
		compiledStage.shaderBlobId = BuildShaderBlobId(
		    node.package->packageId,
		    compiledStage.entryPoint,
		    {},
		    compiledStage.backendName,
		    compiledStage.codegenTarget,
		    compiledStage.format);
	}
	compiledStage.sourceHash = node.sourceHash;
	compiledStage.includeClosureHash = node.includeClosureHash;
	compiledStage.optionsHash = node.optionsHash;
	compiledStage.cacheKey = node.cacheKey.value;
	compiledStage.cacheStatus.assign(cacheStatus);
}