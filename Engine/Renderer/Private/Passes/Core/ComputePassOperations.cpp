#include "PCH.h"

#include "Passes/Core/ComputePassOperations.h"

namespace ComputePassOperations
{
	RenderPassDefinition BuildDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineName,
	    CookedShaderPackageFeatureFlags requiredFeatures)
	{
		return RenderPassDefinition{
		    .PassName = passName,
		    .ShaderPackage = ShaderPackageDefinition{
		        .PackageId = packageId.data(),
		        .ExpectedStages = ShaderStageMask::Compute,
		        .RequiredFeatures = requiredFeatures},
		    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
		    .BindingLayoutDebugName = bindingLayoutName,
		    .PipelineDebugName = pipelineName};
	}
}
