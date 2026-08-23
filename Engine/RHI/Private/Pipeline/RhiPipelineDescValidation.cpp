#include "PCH.h"

#include "Pipeline/RhiPipelineDesc.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Validation/RhiContract.h"

#include <cstdint>

namespace RhiPipelineDescValidation
{
	bool HasVertexBinding(const RhiVertexInputDeclaration& declaration, std::uint8_t binding) noexcept
	{
		for (std::uint32_t index = 0; index < declaration.BindingCount; ++index)
		{
			if (declaration.Bindings[index].Binding == binding)
			{
				return true;
			}
		}
		return false;
	}

	void Require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw Diagnostics::Error(message);
		}
	}

	void RequireShaderStage(const RhiShaderStageDesc& shader, ShaderStage expectedStage, const char* message)
	{
		Require(shader.IsValid() && shader.Shader->Entry->Stage == expectedStage, message);
	}
}

void RhiContract::ValidateGraphicsPipelineDesc(const GraphicsPipelineDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.BindingLayout != nullptr, "Graphics pipeline requires a binding layout.");
	RhiPipelineDescValidation::RequireShaderStage(
	    desc.VertexShader,
	    ShaderStage::Vertex,
	    "Graphics pipeline requires a valid vertex shader stage.");
	if (desc.PixelShader.Shader != nullptr)
	{
		RhiPipelineDescValidation::RequireShaderStage(
		    desc.PixelShader,
		    ShaderStage::Pixel,
		    "Graphics pipeline pixel shader descriptor is invalid or has the wrong stage.");
	}
	RhiPipelineDescValidation::Require(
	    desc.ColorAttachmentCount != 0 || desc.DepthStencilAttachmentFormat != PixelFormat::Unknown,
	    "Graphics pipeline requires at least one color or depth-stencil attachment format.");
	RhiPipelineDescValidation::Require(
	    desc.ColorAttachmentCount <= desc.ColorAttachmentFormats.size(),
	    "Graphics pipeline color attachment count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.VertexInput.BindingCount <= desc.VertexInput.Bindings.size(),
	    "Graphics pipeline vertex binding count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.VertexInput.ElementCount <= desc.VertexInput.Elements.size(),
	    "Graphics pipeline vertex element count exceeds descriptor capacity.");
	RhiPipelineDescValidation::Require(
	    desc.SampleCount == 1 || desc.SampleCount == 2 || desc.SampleCount == 4 || desc.SampleCount == 8,
	    "Graphics pipeline sample count is unsupported by the neutral RHI contract.");
	RhiPipelineDescValidation::Require(
	    desc.Depth.DepthEnable || !desc.Depth.DepthWriteEnable,
	    "Graphics pipeline depth writes require depth testing.");

	for (std::uint32_t index = 0; index < desc.VertexInput.ElementCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    RhiPipelineDescValidation::HasVertexBinding(desc.VertexInput, desc.VertexInput.Elements[index].Binding),
		    "Graphics pipeline vertex input references a missing binding.");
	}

	const bool hasDepthStencilFormat = desc.DepthStencilAttachmentFormat != PixelFormat::Unknown;
	RhiPipelineDescValidation::Require(
	    (!desc.Depth.DepthEnable && !desc.Depth.DepthWriteEnable && !desc.Stencil.StencilEnable) || hasDepthStencilFormat,
	    "Graphics pipeline depth-stencil state requires an attachment format.");
	RhiPipelineDescValidation::Require(
	    !desc.Stencil.StencilEnable || PixelFormatHasStencilAspect(desc.DepthStencilAttachmentFormat),
	    "Graphics pipeline stencil state requires a stencil-capable attachment format.");

	for (std::uint32_t index = 0; index < desc.ColorAttachmentCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    IsColorAttachmentPixelFormat(desc.ColorAttachmentFormats[index]),
		    "Graphics pipeline color attachment format is not color-attachment capable.");
	}
	RhiPipelineDescValidation::Require(
	    !hasDepthStencilFormat || IsDepthStencilPixelFormat(desc.DepthStencilAttachmentFormat),
	    "Graphics pipeline depth-stencil attachment format is not depth-stencil capable.");

	const std::uint32_t blendTargetCount = desc.Blend.IndependentBlendEnable ? desc.ColorAttachmentCount : 1;
	for (std::uint32_t index = 0; index < blendTargetCount; ++index)
	{
		RhiPipelineDescValidation::Require(
		    (desc.Blend.Targets[index].ColorWriteMask & 0xF0u) == 0,
		    "Graphics pipeline color write mask contains unsupported bits.");
	}
}

void RhiContract::ValidateComputePipelineDesc(const ComputePipelineDesc& desc)
{
	RhiPipelineDescValidation::Require(desc.BindingLayout != nullptr, "Compute pipeline requires a binding layout.");
	RhiPipelineDescValidation::RequireShaderStage(
	    desc.ComputeShader,
	    ShaderStage::Compute,
	    "Compute pipeline requires a valid compute shader stage.");
}
