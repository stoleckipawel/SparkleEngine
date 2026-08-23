#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureMomentChain.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposureDownsampleSceneShader.h"
#include "Passes/PostProcessing/ExposureDownsampleTextureShader.h"
#include "Passes/PostProcessing/ExposureReduceSceneShader.h"
#include "Passes/PostProcessing/ExposureReduceTextureShader.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <algorithm>
#include <string>

namespace ExposureMomentChain
{
	constexpr PixelFormat kMomentFormat = PixelFormat::R32G32B32A32_Float;

	Texture CreateTexture(FrameGraphBuilder& builder, const char* prefix, std::uint32_t level, std::uint32_t width, std::uint32_t height)
	{
		const std::string name = std::string(prefix) + std::to_string(level);
		return Texture{
		    .TextureHandle = builder.CreateTexture(FrameGraphTextureDesc::CreateColor(name, width, height, kMomentFormat)),
		    .Width = width,
		    .Height = height};
	}

	void AddReduceScenePass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const Texture& output)
	{
		auto& parameters = builder.AllocParameters<ExposureReduceSceneCS>();
		parameters->SceneColor = builder.CreateSRV(sceneColor);
		parameters->LuminanceMomentsOutput = builder.CreateUAV(output.TextureHandle);
		builder.DispatchAsync<ExposureReduceSceneCS>(
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 16u), MathUtils::DivideRoundUp(output.Height, 16u), 1u});
	}

	void AddReduceTexturePass(FrameGraphBuilder& builder, const Texture& input, const Texture& output)
	{
		auto& parameters = builder.AllocParameters<ExposureReduceTextureCS>();
		parameters->LuminanceMomentsInput = builder.CreateSRV(input.TextureHandle);
		parameters->LuminanceMomentsOutput = builder.CreateUAV(output.TextureHandle);
		builder.DispatchAsync<ExposureReduceTextureCS>(
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 16u), MathUtils::DivideRoundUp(output.Height, 16u), 1u});
	}

	void AddDownsampleScenePass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const Texture& output)
	{
		auto& parameters = builder.AllocParameters<ExposureDownsampleSceneCS>();
		parameters->SceneColor = builder.CreateSRV(sceneColor);
		parameters->LuminanceMomentsOutput = builder.CreateUAV(output.TextureHandle);
		builder.DispatchAsync<ExposureDownsampleSceneCS>(
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 8u), MathUtils::DivideRoundUp(output.Height, 8u), 1u});
	}

	void AddDownsampleTexturePass(FrameGraphBuilder& builder, const Texture& input, const Texture& output)
	{
		auto& parameters = builder.AllocParameters<ExposureDownsampleTextureCS>();
		parameters->LuminanceMomentsInput = builder.CreateSRV(input.TextureHandle);
		parameters->LuminanceMomentsOutput = builder.CreateUAV(output.TextureHandle);
		builder.DispatchAsync<ExposureDownsampleTextureCS>(
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(output.Width, 8u), MathUtils::DivideRoundUp(output.Height, 8u), 1u});
	}

	Texture AddReduction(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameGraphTextureHandle sceneColor)
	{
		std::uint32_t width = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Width, 16u), 1u);
		std::uint32_t height = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Height, 16u), 1u);
		Texture current = CreateTexture(builder, "ExposureReductionMoments", 0u, width, height);
		AddReduceScenePass(builder, sceneColor, current);

		std::uint32_t level = 1u;
		while (current.Width > 1u || current.Height > 1u)
		{
			width = (std::max) (MathUtils::DivideRoundUp(current.Width, 16u), 1u);
			height = (std::max) (MathUtils::DivideRoundUp(current.Height, 16u), 1u);
			Texture next = CreateTexture(builder, "ExposureReductionMoments", level, width, height);
			AddReduceTexturePass(builder, current, next);
			current = next;
			++level;
		}

		return current;
	}

	Texture AddDownsample(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameGraphTextureHandle sceneColor)
	{
		std::uint32_t width = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Width, 2u), 1u);
		std::uint32_t height = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Height, 2u), 1u);
		Texture current = CreateTexture(builder, "ExposureDownsampleMoments", 0u, width, height);
		AddDownsampleScenePass(builder, sceneColor, current);

		std::uint32_t level = 1u;
		while (current.Width > 1u || current.Height > 1u)
		{
			width = (std::max) (MathUtils::DivideRoundUp(current.Width, 2u), 1u);
			height = (std::max) (MathUtils::DivideRoundUp(current.Height, 2u), 1u);
			Texture next = CreateTexture(builder, "ExposureDownsampleMoments", level, width, height);
			AddDownsampleTexturePass(builder, current, next);
			current = next;
			++level;
		}

		return current;
	}
}
