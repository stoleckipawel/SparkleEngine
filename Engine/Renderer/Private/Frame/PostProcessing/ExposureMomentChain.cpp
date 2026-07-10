#include "../../PCH.h"
#include "Frame/PostProcessing/ExposureMomentChain.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposureDownsampleScenePass.h"
#include "Passes/PostProcessing/ExposureDownsampleTexturePass.h"
#include "Passes/PostProcessing/ExposureReduceScenePass.h"
#include "Passes/PostProcessing/ExposureReduceTexturePass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
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
		auto& parameters = builder.AllocPassParameters<ExposureReduceScenePass>();
		ExposureReduceScenePass::DeclareResources(builder, sceneColor, output.TextureHandle, parameters);
		builder.AddSizedComputeShaderPass<ExposureReduceScenePass>(parameters, output.Width, output.Height);
	}

	void AddReduceTexturePass(FrameGraphBuilder& builder, const Texture& input, const Texture& output)
	{
		auto& parameters = builder.AllocPassParameters<ExposureReduceTexturePass>();
		ExposureReduceTexturePass::DeclareResources(builder, input.TextureHandle, output.TextureHandle, parameters);
		builder.AddSizedComputeShaderPass<ExposureReduceTexturePass>(parameters, output.Width, output.Height);
	}

	void AddDownsampleScenePass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const Texture& output)
	{
		auto& parameters = builder.AllocPassParameters<ExposureDownsampleScenePass>();
		ExposureDownsampleScenePass::DeclareResources(builder, sceneColor, output.TextureHandle, parameters);
		builder.AddSizedComputeShaderPass<ExposureDownsampleScenePass>(parameters, output.Width, output.Height);
	}

	void AddDownsampleTexturePass(FrameGraphBuilder& builder, const Texture& input, const Texture& output)
	{
		auto& parameters = builder.AllocPassParameters<ExposureDownsampleTexturePass>();
		ExposureDownsampleTexturePass::DeclareResources(builder, input.TextureHandle, output.TextureHandle, parameters);
		builder.AddSizedComputeShaderPass<ExposureDownsampleTexturePass>(parameters, output.Width, output.Height);
	}

	Texture AddReduction(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameGraphTextureHandle sceneColor)
	{
		std::uint32_t width = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Width, ExposureReduceScenePass::ThreadGroupSizeX), 1u);
		std::uint32_t height = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Height, ExposureReduceScenePass::ThreadGroupSizeY), 1u);
		Texture current = CreateTexture(builder, "ExposureReductionMoments", 0u, width, height);
		AddReduceScenePass(builder, sceneColor, current);

		std::uint32_t level = 1u;
		while (current.Width > 1u || current.Height > 1u)
		{
			width = (std::max) (MathUtils::DivideRoundUp(current.Width, ExposureReduceTexturePass::ThreadGroupSizeX), 1u);
			height = (std::max) (MathUtils::DivideRoundUp(current.Height, ExposureReduceTexturePass::ThreadGroupSizeY), 1u);
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
