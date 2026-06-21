#include "../../PCH.h"
#include "Passes/Deferred/IndirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Resources/Texture.h"
#include "Textures/TextureManager.h"

#include <cassert>

IndirectLightingPass::IndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

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

const IndirectLightingPass::ParameterMetadata& IndirectLightingPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& IndirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "IndirectLightingShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::IndirectLighting.data(),
	        .BindingLayoutId = RendererShaderPackages::IndirectLighting.data(),
	        .ExpectedStages = ShaderStageMask::Compute},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"IndirectLighting_BindingLayout",
	    .PipelineStateDebugName = L"IndirectLighting_PipelineState"};
	return definition;
}

void IndirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->IndirectDiffuse = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->IndirectSubsurface = builder.CreateUAV(lighting.IndirectSubsurface);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	parameters->SkyTexture = builder.CreateSRV(gbuffer.Normal);
	parameters->SamplerLinearNoMipClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Point,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
}

void IndirectLightingPass::SetParameters(ParameterInstance& parameters, const PassRuntimeServices& passRuntimeServices) const
{
	parameters->SamplerLinearNoMipClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Point,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	const bool valid = parameters.Sync();
	assert(valid);
}

RhiDescriptorTableBinding IndirectLightingPass::GetSkyTextureBinding(const PassRuntimeServices& passRuntimeServices) const noexcept
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

void IndirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.RuntimeServices);
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkyTexture", GetSkyTextureBinding(context.RuntimeServices));
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchAvailableComputePassWithRuntime<IndirectLightingPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    &overrides,
	    PassName);
	assert(dispatched);
}
