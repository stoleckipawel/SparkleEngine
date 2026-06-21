#include "../../PCH.h"
#include "Passes/Deferred/SkyPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Resources/Texture.h"
#include "Textures/TextureManager.h"

#include <cassert>

SkyPass::SkyPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

namespace
{
	const Texture* ResolveSkyTexture(const TextureManager* textureManager) noexcept
	{
		if (textureManager == nullptr)
		{
			return nullptr;
		}

		if (const Texture* skyTexture = textureManager->GetTexture(TextureId::SkyCubemap))
		{
			return skyTexture;
		}

		return textureManager->GetTexture(TextureId::Checker);
	}

}

const SkyPass::ParameterMetadata& SkyPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

const RenderPassDefinition& SkyPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "SkyShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::Sky.data(),
	        .BindingLayoutId = RendererShaderPackages::Sky.data(),
	        .ExpectedStages = ShaderStageMask::Compute},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"Sky_BindingLayout",
	    .PipelineStateDebugName = L"Sky_PipelineState"};
	return definition;
}

void SkyPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateUAV(sceneTargets.SceneColor);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
}

void SkyPass::SetParameters(ParameterInstance& parameters, const RenderViewData& viewData, const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->SkyTexture = GetSkyTextureBinding(passRuntimeServices);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	const bool valid = parameters.Sync();
	assert(valid);
}

RhiDescriptorTableBinding SkyPass::GetSkyTextureBinding(const PassRuntimeServices& passRuntimeServices) const noexcept
{
	const Texture* skyTexture = ResolveSkyTexture(passRuntimeServices.Textures);
	if (skyTexture == nullptr)
	{
		return {};
	}

	RenderHardwareInterface& renderHardwareInterface = passRuntimeServices.HardwareInterface;
	if (!m_skyTextureBindingSet)
	{
		m_skyTextureBindingSet = renderHardwareInterface.GetDescriptorService().CreateBindingSet(
		    RenderBindingSetDesc{.DescriptorType = ERhiDescriptorAllocatorType::ShaderResource, .DescriptorCount = 1u});
	}

	if (!m_skyTextureBindingSet || !*m_skyTextureBindingSet)
	{
		return {};
	}

	if (m_cachedSkyTexture != skyTexture)
	{
		skyTexture->WriteShaderResourceView(m_skyTextureBindingSet->GetCpuDescriptorHandle());
		m_cachedSkyTexture = skyTexture;
	}

	return m_skyTextureBindingSet->GetTableBinding();
}

void SkyPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<SkyPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    PassName);
	assert(dispatched);
}
