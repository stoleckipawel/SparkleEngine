#include "../../PCH.h"
#include "Passes/Presentation/PresentScenePass.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

PresentScenePass::PresentScenePass(const RasterPassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const PresentScenePass::ParameterMetadata& PresentScenePass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Raster, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

const RenderPassDefinition& PresentScenePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "PresentSceneShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::PresentScene.data(),
	        .BindingLayoutId = RendererShaderPackages::PresentScene.data(),
	        .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Graphics,
	    .AllowInputAssemblerInputLayout = true,
	    .BindingLayoutDebugName = L"PresentScene_BindingLayout",
	    .PipelineStateDebugName = L"PresentScene_PipelineState",
	    .Graphics = RenderPassGraphicsPipelineDefinition{
	        .CullMode = ERhiCullMode::None,
	        .DepthTest = RhiDepthTestDesc{.DepthEnable = false, .DepthWriteEnable = false},
	        .RenderTargetFormats = {PixelFormat::Unknown},
	        .RenderTargetCount = 1,
	        .UsePresentColorFormat = true,
	        .DepthStencilFormat = PixelFormat::Unknown}};
	return definition;
}

void PresentScenePass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    ParameterInstance& parameters)
{
	parameters->BackBuffer = builder.CreateRenderTarget(sceneTargets.BackBuffer);
	parameters->SceneColor = builder.CreateSRV(sceneTargets.FinalSceneColor);
	parameters->SamplerLinearClamp = RhiSamplerDesc{};
}

void PresentScenePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const bool valid = parameters.Sync();
	assert(valid);

	context.Commands.SetViewport(context.Frame.mainView.viewport);
	context.Commands.SetScissorRect(context.Frame.mainView.scissorRect);
	context.Resources.BindRenderTarget(context.Commands, parameters->BackBuffer[0]);

	const bool bound = RasterShaderPass<PresentScenePass::Parameters>::Bind(
	    context.Resources,
	    context.Commands,
	    &context.RuntimeServices.HardwareInterface,
	    m_runtime.BindingLayout,
	    m_runtime.PipelineState,
	    parameters,
	    nullptr,
	    0,
	    nullptr,
	    PassName);
	assert(bound);

	PassUtilities::DrawFullscreenTriangle(context.Commands);
}
