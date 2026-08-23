#include "PCH.h"
#include "Pipeline/GraphicsPipelineMaterialization.h"

#include "Core/Public/Hash/HashUtils.h"

namespace GraphicsPipelineMaterialization
{
	template <typename TValue> void Append(std::uint64_t& hash, const TValue& value) noexcept
	{
		hash = Hash::ContinueFnv1a64Value(hash, value);
	}

	void AppendVertexInput(std::uint64_t& hash, const RhiVertexInputDeclaration& declaration) noexcept
	{
		Append(hash, declaration.BindingCount);
		Append(hash, declaration.ElementCount);
		for (std::uint32_t index = 0; index < declaration.BindingCount; ++index)
		{
			const RhiVertexInputBinding& binding = declaration.Bindings[index];
			Append(hash, binding.Binding);
			Append(hash, binding.StrideInBytes);
			Append(hash, binding.PerInstance);
		}
		for (std::uint32_t index = 0; index < declaration.ElementCount; ++index)
		{
			const RhiVertexInputElement& element = declaration.Elements[index];
			Append(hash, element.Semantic);
			Append(hash, element.SemanticIndex);
			Append(hash, element.Location);
			Append(hash, element.Binding);
			Append(hash, element.Format);
			Append(hash, element.OffsetInBytes);
		}
	}

	void AppendBlend(std::uint64_t& hash, const RhiBlendState& blend) noexcept
	{
		Append(hash, blend.AlphaToCoverageEnable);
		Append(hash, blend.IndependentBlendEnable);
		for (const RhiBlendTargetState& target : blend.Targets)
		{
			Append(hash, target.BlendEnable);
			Append(hash, target.SourceColor);
			Append(hash, target.DestinationColor);
			Append(hash, target.ColorOperation);
			Append(hash, target.SourceAlpha);
			Append(hash, target.DestinationAlpha);
			Append(hash, target.AlphaOperation);
			Append(hash, target.ColorWriteMask);
		}
	}

	void AppendStencil(std::uint64_t& hash, const RhiStencilState& stencil) noexcept
	{
		Append(hash, stencil.StencilEnable);
		Append(hash, stencil.StencilReadMask);
		Append(hash, stencil.StencilWriteMask);
		Append(hash, stencil.FrontFaceStencilFunc);
		Append(hash, stencil.FrontFaceStencilFailOp);
		Append(hash, stencil.FrontFaceStencilDepthFailOp);
		Append(hash, stencil.FrontFaceStencilPassOp);
		Append(hash, stencil.BackFaceStencilFunc);
		Append(hash, stencil.BackFaceStencilFailOp);
		Append(hash, stencil.BackFaceStencilDepthFailOp);
		Append(hash, stencil.BackFaceStencilPassOp);
	}
}

std::size_t GraphicsPipelineKeyHash::operator()(const GraphicsPipelineKey& key) const noexcept
{
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	GraphicsPipelineMaterialization::Append(hash, key.ShaderGeneration);
	GraphicsPipelineMaterialization::Append(hash, key.VertexShaderCode);
	GraphicsPipelineMaterialization::Append(hash, key.PixelShaderCode);
	GraphicsPipelineMaterialization::Append(hash, key.BindingLayout);
	GraphicsPipelineMaterialization::AppendBlend(hash, key.Request.Blend);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Rasterizer.FillMode);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Rasterizer.CullMode);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Rasterizer.FrontFaceWinding);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Rasterizer.DepthClipEnable);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Depth.DepthEnable);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Depth.DepthWriteEnable);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Depth.DepthFunc);
	GraphicsPipelineMaterialization::AppendStencil(hash, key.Request.Stencil);
	GraphicsPipelineMaterialization::Append(hash, key.Request.PrimitiveTopology);
	GraphicsPipelineMaterialization::AppendVertexInput(hash, key.Request.VertexInput);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Attachments.ColorCount);
	for (std::uint32_t index = 0; index < key.Request.Attachments.ColorCount; ++index)
	{
		GraphicsPipelineMaterialization::Append(hash, key.Request.Attachments.ColorFormats[index]);
	}
	GraphicsPipelineMaterialization::Append(hash, key.Request.Attachments.DepthStencilAttachmentFormat);
	GraphicsPipelineMaterialization::Append(hash, key.Request.Attachments.SampleCount);
	return static_cast<std::size_t>(hash);
}

GraphicsPipelineRequest BuildGraphicsPipelineRequest(
    const RasterPassRenderState& passState,
    const RhiRasterizerState& rasterizer,
    RhiPrimitiveTopology primitiveTopology,
    const RhiVertexInputDeclaration& vertexInput,
    const GraphicsAttachmentSignature& attachments) noexcept
{
	return GraphicsPipelineRequest{
	    .Blend = passState.GetBlend(),
	    .Rasterizer = rasterizer,
	    .Depth = passState.GetDepth(),
	    .Stencil = passState.GetStencil(),
	    .PrimitiveTopology = primitiveTopology,
	    .VertexInput = vertexInput,
	    .Attachments = attachments};
}

GraphicsPipelineDesc BuildGraphicsPipelineDesc(
    const GraphicsPipelineRequest& request,
    const RenderBindingLayout& bindingLayout,
    const ResolvedShader& vertexShader,
    const ResolvedShader& pixelShader,
    const wchar_t* debugName) noexcept
{
	return GraphicsPipelineDesc{
	    .BindingLayout = &bindingLayout,
	    .VertexShader = RhiShaderStageDesc{&vertexShader},
	    .PixelShader = RhiShaderStageDesc{&pixelShader},
	    .Blend = request.Blend,
	    .Rasterizer = request.Rasterizer,
	    .Depth = request.Depth,
	    .Stencil = request.Stencil,
	    .PrimitiveTopology = request.PrimitiveTopology,
	    .VertexInput = request.VertexInput,
	    .ColorAttachmentFormats = request.Attachments.ColorFormats,
	    .ColorAttachmentCount = request.Attachments.ColorCount,
	    .DepthStencilAttachmentFormat = request.Attachments.DepthStencilAttachmentFormat,
	    .SampleCount = request.Attachments.SampleCount,
	    .DebugName = debugName};
}
