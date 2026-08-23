#include "PCH.h"

#include "Pipeline/RhiShaderBindingReflection.h"

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/LoadedShaderPackage.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <format>
#include <ranges>
#include <string_view>

static const auto g_rhiShaderBindingReflectionLogger = Logging::GetOrCreateLogger("RHI.ShaderBindingReflection");

class RhiShaderBindingReflectionImpl final
{
public:
	static std::vector<RhiReflectedBindingLocation> ResolveLocations(
	    const LoadedShaderPackage& shaderPackage,
	    const PassParameterLayout& parameterLayout,
	    const CookedShaderBindingRecord& bindingRecord,
	    CookedShaderBinaryFormat binaryFormat)
	{
		const std::string_view bindingName = shaderPackage.ResolveString(bindingRecord.Name);
		if (!parameterLayout.HasParameter(bindingName))
		{
			Fail(std::format("Shader binding '{}' has no matching parameter in layout '{}'.", bindingName, parameterLayout.GetDebugName()));
		}

		const std::vector<CookedShaderBinaryRecord>& binaryRecords = shaderPackage.GetBinaryRecords();
		const std::vector<CookedShaderReflectionRecord>& reflectionRecords = shaderPackage.GetReflectionRecords();
		const std::vector<CookedShaderResourceBindingRecord>& resourceBindings = shaderPackage.GetResourceBindings();
		std::vector<RhiReflectedBindingLocation> locations;

		for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size() && reflectionIndex < binaryRecords.size();
		    ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (!shaderPackage.IsRuntimeBinary(binaryRecord, binaryFormat)
			    || !HasAnyShaderStageMask(bindingRecord.VisibilityMask, ToShaderStageMask(binaryRecord.Stage)))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t resourceBindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (resourceBindingIndex >= resourceBindings.size())
				{
					Fail(std::format("Shader reflection range for '{}' exceeds the cooked resource-binding table.", bindingName));
				}

				const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[resourceBindingIndex];
				if (!ResourceKindMatches(resourceBinding.Kind, bindingRecord.SemanticKind))
				{
					continue;
				}

				const std::string_view resourceName =
				    shaderPackage.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (resourceName != bindingName)
				{
					continue;
				}

				const RhiBindingPoint bindingPoint{.Set = resourceBinding.Set, .Binding = resourceBinding.Slot};
				const auto existing = std::ranges::find_if(
				    locations,
				    [bindingPoint](const RhiReflectedBindingLocation& location)
				    { return location.BindingPoint.Set == bindingPoint.Set && location.BindingPoint.Binding == bindingPoint.Binding; });
				if (existing != locations.end())
				{
					existing->VisibilityMask |= ToShaderStageMask(binaryRecord.Stage);
				}
				else
				{
					locations.push_back(
					    RhiReflectedBindingLocation{.BindingPoint = bindingPoint, .VisibilityMask = ToShaderStageMask(binaryRecord.Stage)});
				}
			}
		}

		if (locations.empty())
		{
			Fail(std::format("Shader parameter '{}' did not resolve to a reflected resource.", bindingName));
		}

		return locations;
	}

private:
	static bool ResourceKindMatches(CookedShaderResourceKind kind, ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return kind == CookedShaderResourceKind::ConstantBuffer || kind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture:
				return kind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return kind == CookedShaderResourceKind::StructuredBuffer || kind == CookedShaderResourceKind::ByteAddressBuffer
				    || kind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return kind == CookedShaderResourceKind::AccelerationStructure;
			case ShaderParameterSemanticKind::RWTexture:
				return kind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return kind == CookedShaderResourceKind::RWStructuredBuffer || kind == CookedShaderResourceKind::RWByteAddressBuffer
				    || kind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet:
				return kind == CookedShaderResourceKind::Sampler;
			default:
				return false;
		}
	}

	[[noreturn]] static void Fail(std::string_view message)
	{
		Diagnostics::Fatal(g_rhiShaderBindingReflectionLogger, __FILE__, __LINE__, message);
	}
};

std::vector<RhiReflectedBindingLocation> RhiShaderBindingReflection::ResolveLocations(
    const LoadedShaderPackage& shaderPackage,
    const PassParameterLayout& parameterLayout,
    const CookedShaderBindingRecord& bindingRecord,
    CookedShaderBinaryFormat binaryFormat)
{
	return RhiShaderBindingReflectionImpl::ResolveLocations(shaderPackage, parameterLayout, bindingRecord, binaryFormat);
}
