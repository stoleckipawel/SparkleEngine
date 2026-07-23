#include "PCH.h"

#include "Shaders/CookedShaderBindingRules.h"

namespace CookedShaderBindingRules
{
	bool HasAllStages(ShaderStageMask value, ShaderStageMask flags) noexcept
	{
		return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(flags)) ==
		       static_cast<std::uint8_t>(flags);
	}

	ShaderStageMask ToPackageStageMask(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::Vertex: return ShaderStageMask::Vertex;
			case ShaderStageVisibility::Pixel: return ShaderStageMask::Pixel;
			case ShaderStageVisibility::Compute: return ShaderStageMask::Compute;
			case ShaderStageVisibility::AllGraphics: return ShaderStageMask::Vertex | ShaderStageMask::Pixel;
			case ShaderStageVisibility::All:
				return ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute;
			case ShaderStageVisibility::None:
			default: return ShaderStageMask::None;
		}
	}

	bool ResourceKindMatchesSemantic(
	    CookedShaderResourceKind resourceKind,
	    ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return resourceKind == CookedShaderResourceKind::ConstantBuffer ||
				       resourceKind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture:
				return resourceKind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return resourceKind == CookedShaderResourceKind::StructuredBuffer ||
				       resourceKind == CookedShaderResourceKind::ByteAddressBuffer ||
				       resourceKind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::RWTexture:
			case ShaderParameterSemanticKind::RenderTarget:
			case ShaderParameterSemanticKind::DepthTarget:
				return resourceKind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return resourceKind == CookedShaderResourceKind::RWStructuredBuffer ||
				       resourceKind == CookedShaderResourceKind::RWByteAddressBuffer ||
				       resourceKind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet:
				return resourceKind == CookedShaderResourceKind::Sampler;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return resourceKind == CookedShaderResourceKind::AccelerationStructure;
		}

		return false;
	}
}  // namespace CookedShaderBindingRules
