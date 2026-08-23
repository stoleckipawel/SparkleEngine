#pragma once

#include "Pipeline/RasterPassRenderState.h"

#include "RHI/Public/Shaders/GlobalShaderMap.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct GraphicsAttachmentSignature final
{
	std::array<PixelFormat, 8> ColorFormats = {};
	std::uint32_t ColorCount = 0;
	PixelFormat DepthStencilAttachmentFormat = PixelFormat::Unknown;
	std::uint8_t SampleCount = 1;

	bool operator==(const GraphicsAttachmentSignature& other) const noexcept
	{
		if (ColorCount != other.ColorCount || DepthStencilAttachmentFormat != other.DepthStencilAttachmentFormat
		    || SampleCount != other.SampleCount)
		{
			return false;
		}
		if (ColorCount > ColorFormats.size())
		{
			return ColorFormats == other.ColorFormats;
		}
		for (std::uint32_t index = 0; index < ColorCount; ++index)
		{
			if (ColorFormats[index] != other.ColorFormats[index])
			{
				return false;
			}
		}
		return true;
	}
};

struct GraphicsPipelineRequest final
{
	RhiBlendState Blend = {};
	RhiRasterizerState Rasterizer = {};
	RhiDepthState Depth = {};
	RhiStencilState Stencil = {};
	RhiPrimitiveTopology PrimitiveTopology = RhiPrimitiveTopology::TriangleList;
	RhiVertexInputDeclaration VertexInput = {};
	GraphicsAttachmentSignature Attachments = {};

	bool operator==(const GraphicsPipelineRequest&) const noexcept = default;
};

struct GraphicsPipelineKey final
{
	std::uint64_t ShaderGeneration = 0;
	ShaderCodeHash VertexShaderCode = 0;
	ShaderCodeHash PixelShaderCode = 0;
	ShaderParameterSignature BindingLayout = 0;
	GraphicsPipelineRequest Request = {};

	bool operator==(const GraphicsPipelineKey&) const noexcept = default;
};

struct GraphicsPipelineKeyHash final
{
	std::size_t operator()(const GraphicsPipelineKey& key) const noexcept;
};

GraphicsPipelineRequest BuildGraphicsPipelineRequest(
    const RasterPassRenderState& passState,
    const RhiRasterizerState& rasterizer,
    RhiPrimitiveTopology primitiveTopology,
    const RhiVertexInputDeclaration& vertexInput,
    const GraphicsAttachmentSignature& attachments) noexcept;

GraphicsPipelineDesc BuildGraphicsPipelineDesc(
    const GraphicsPipelineRequest& request,
    const RenderBindingLayout& bindingLayout,
    const ResolvedShader& vertexShader,
    const ResolvedShader& pixelShader,
    const wchar_t* debugName) noexcept;
