#include "PCH.h"

#include "Passes/Core/ComputePassUtilities.h"

namespace ComputePassUtilities
{
	RenderPassDefinition BuildDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineStateName,
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
		    .PipelineStateDebugName = pipelineStateName};
	}
}
