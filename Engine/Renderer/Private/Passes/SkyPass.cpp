#include "../PCH.h"
#include "Passes/SkyPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/Execution/RenderGraphPassContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "RHI/Public/Resources/Texture.h"
#include "Textures/TextureManager.h"

#include <cassert>

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

	RhiDescriptorTableBinding GetSkyTextureBinding(const RenderPassContext& renderPassContext) noexcept
	{
		const Texture* skyTexture = ResolveSkyTexture(renderPassContext.Textures);
		if (skyTexture == nullptr)
		{
			return {};
		}

		static RhiDescriptorTableHandle skyTextureTable = {};
		static const Texture* cachedTexture = nullptr;
		RenderHardwareInterface& renderHardwareInterface = renderPassContext.HardwareInterface;
		if (!skyTextureTable)
		{
			skyTextureTable = renderHardwareInterface.AllocateDescriptorTable(ERhiDescriptorHeapType::ShaderResource, 1u);
		}

		if (!skyTextureTable)
		{
			return {};
		}

		if (cachedTexture != skyTexture)
		{
			skyTexture->WriteShaderResourceView(renderHardwareInterface.GetDescriptorTableCpuHandle(skyTextureTable));
			cachedTexture = skyTexture;
		}

		return RhiDescriptorTableBinding{skyTextureTable, 0u};
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

ShaderPackageDefinition SkyPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{.PackageId = PassName, .BindingLayoutId = PassName, .ExpectedStages = ShaderStageMask::Compute};
}

void SkyPass::DeclareResources(
    FrameGraph& frameGraph,
    const SceneTargets& sceneTargets,
    const GBufferTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = frameGraph.CreateUAV(sceneTargets.SceneColor);
	parameters->GBufferDeviceZ = frameGraph.CreateSRV(gbuffer.DeviceZ);
}

void SkyPass::SetParameters(ParameterInstance& parameters, const RenderViewData& viewData, const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	const bool valid = parameters.Sync();
	assert(valid);
}

void SkyPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.Sky.Execute");

	const SkyPassRuntime& runtime = context.Runtime.GetPassRuntime<SkyPass>();
	SetParameters(parameters, context.Frame.mainView, context.Runtime);
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkyTexture", GetSkyTextureBinding(context.Runtime));
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchAvailableComputePassWithRuntime<SkyPass>(
	    context.Graph,
	    context.Commands,
	    context.Runtime.HardwareInterface,
	    runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    &overrides,
	    PassName);
	assert(dispatched);
}