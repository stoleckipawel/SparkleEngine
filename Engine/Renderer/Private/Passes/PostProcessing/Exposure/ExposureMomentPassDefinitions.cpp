#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureMomentPassDefinitions.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/Exposure/ExposureDownsampleSceneShader.h"
#include "Passes/PostProcessing/Exposure/ExposureDownsampleTextureShader.h"
#include "Passes/PostProcessing/Exposure/ExposureReduceSceneShader.h"
#include "Passes/PostProcessing/Exposure/ExposureReduceTextureShader.h"
void AddExposureSceneReductionPass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const ExposureMomentTexture& output)
{
	auto& parameters = builder.AllocParameters<ExposureReduceSceneCS>();
	parameters->SceneColor = builder.CreateSRV(sceneColor);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(output.Handle);
	builder.Dispatch<ExposureReduceSceneCS>(
	    parameters,
	    ComputeDispatchDesc{output.Width, output.Height, 1u},
	    EFrameGraphQueuePreference::AsyncCompute);
}

void AddExposureTextureReductionPass(FrameGraphBuilder& builder, const ExposureMomentTexture& input, const ExposureMomentTexture& output)
{
	auto& parameters = builder.AllocParameters<ExposureReduceTextureCS>();
	parameters->LuminanceMomentsInput = builder.CreateSRV(input.Handle);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(output.Handle);
	builder.Dispatch<ExposureReduceTextureCS>(
	    parameters,
	    ComputeDispatchDesc{output.Width, output.Height, 1u},
	    EFrameGraphQueuePreference::AsyncCompute);
}

void AddExposureSceneDownsamplePass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const ExposureMomentTexture& output)
{
	auto& parameters = builder.AllocParameters<ExposureDownsampleSceneCS>();
	parameters->SceneColor = builder.CreateSRV(sceneColor);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(output.Handle);
	builder.Dispatch<ExposureDownsampleSceneCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 8u), MathUtils::DivideRoundUp(output.Height, 8u), 1u},
	    EFrameGraphQueuePreference::AsyncCompute);
}

void AddExposureTextureDownsamplePass(FrameGraphBuilder& builder, const ExposureMomentTexture& input, const ExposureMomentTexture& output)
{
	auto& parameters = builder.AllocParameters<ExposureDownsampleTextureCS>();
	parameters->LuminanceMomentsInput = builder.CreateSRV(input.Handle);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(output.Handle);
	builder.Dispatch<ExposureDownsampleTextureCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 8u), MathUtils::DivideRoundUp(output.Height, 8u), 1u},
	    EFrameGraphQueuePreference::AsyncCompute);
}
