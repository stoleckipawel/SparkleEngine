#include "PCH.h"

#include "Pipeline/RhiShaderBindingReflection.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <format>

static const auto g_rhiShaderBindingReflectionLogger = Logging::GetOrCreateLogger("RHI.ShaderBindingReflection");

class RhiShaderBindingReflectionImplementation final
{
public:
	static bool ResourceKindMatches(CookedShaderResourceKind kind, ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return kind == CookedShaderResourceKind::ConstantBuffer || kind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture: return kind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return kind == CookedShaderResourceKind::StructuredBuffer || kind == CookedShaderResourceKind::ByteAddressBuffer
				    || kind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::AccelerationStructure: return kind == CookedShaderResourceKind::AccelerationStructure;
			case ShaderParameterSemanticKind::RWTexture: return kind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return kind == CookedShaderResourceKind::RWStructuredBuffer || kind == CookedShaderResourceKind::RWByteAddressBuffer
				    || kind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet: return kind == CookedShaderResourceKind::Sampler;
			default: return false;
		}
	}

	[[noreturn]] static void Fail(std::string_view message)
	{
		Diagnostics::Fatal(g_rhiShaderBindingReflectionLogger, __FILE__, __LINE__, message);
	}
};

std::vector<RhiReflectedBindingLocation> RhiShaderBindingReflection::ResolveLocations(
    std::span<const ResolvedShader> shaders,
    const PassParameterLayout& parameterLayout,
    std::string_view bindingName,
    ShaderParameterSemanticKind semanticKind)
{
	if (!parameterLayout.HasParameter(bindingName))
	{
		RhiShaderBindingReflectionImplementation::Fail(
		    std::format("Shader binding '{}' has no matching parameter in layout '{}'.", bindingName, parameterLayout.GetDebugName()));
	}
	std::vector<RhiReflectedBindingLocation> locations;
	for (const ResolvedShader& shader : shaders)
	{
		if (!shader.IsValid())
		{
			RhiShaderBindingReflectionImplementation::Fail("Binding reflection received an unresolved shader reference.");
		}
		const CookedShaderReflectionRecord& reflection = shader.Map->GetReflection(*shader.Entry);
		const auto resources = shader.Map->GetResourceBindings();
		for (std::uint32_t index = 0; index < reflection.ResourceBindingCount; ++index)
		{
			const CookedShaderResourceBindingRecord& resource = resources[reflection.ResourceBindingOffset + index];
			if (!RhiShaderBindingReflectionImplementation::ResourceKindMatches(resource.Kind, semanticKind)
			    || shader.Map->ResolveString(ShaderMapStringRef{resource.NameOffsetInBytes, resource.NameSizeInBytes}) != bindingName)
			{
				continue;
			}
			const RhiBindingPoint point{.Set = resource.Set, .Binding = resource.Slot};
			const auto existing = std::ranges::find_if(
			    locations,
			    [point](const RhiReflectedBindingLocation& value)
			    { return value.BindingPoint.Set == point.Set && value.BindingPoint.Binding == point.Binding; });
			if (existing == locations.end())
			{
				locations.push_back(
				    RhiReflectedBindingLocation{
				        .BindingPoint = point,
				        .VisibilityMask = ToShaderStageMask(shader.Entry->Stage)});
			}
			else
			{
				existing->VisibilityMask |= ToShaderStageMask(shader.Entry->Stage);
			}
		}
	}
	if (locations.empty())
	{
		RhiShaderBindingReflectionImplementation::Fail(
		    std::format("Shader parameter '{}' did not resolve to reflected code.", bindingName));
	}
	return locations;
}
