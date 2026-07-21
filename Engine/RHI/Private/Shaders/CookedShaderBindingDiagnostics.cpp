#include "PCH.h"

#include "CookedShaderBindingDiagnostics.h"

#include "CookedShaderBindingRules.h"

#include <format>

namespace
{
	const char* FormatResourceKindName(CookedShaderResourceKind kind) noexcept
	{
		switch (kind)
		{
			case CookedShaderResourceKind::ConstantBuffer: return "ConstantBuffer";
			case CookedShaderResourceKind::Texture: return "Texture";
			case CookedShaderResourceKind::StructuredBuffer: return "StructuredBuffer";
			case CookedShaderResourceKind::ByteAddressBuffer: return "ByteAddressBuffer";
			case CookedShaderResourceKind::TypedBuffer: return "TypedBuffer";
			case CookedShaderResourceKind::RWTexture: return "RWTexture";
			case CookedShaderResourceKind::RWStructuredBuffer: return "RWStructuredBuffer";
			case CookedShaderResourceKind::RWByteAddressBuffer: return "RWByteAddressBuffer";
			case CookedShaderResourceKind::RWTypedBuffer: return "RWTypedBuffer";
			case CookedShaderResourceKind::Sampler: return "Sampler";
			case CookedShaderResourceKind::AccelerationStructure: return "AccelerationStructure";
			case CookedShaderResourceKind::PushConstantBlock: return "PushConstantBlock";
			case CookedShaderResourceKind::Unknown:
			default: return "Unknown";
		}
	}

	const char* FormatResourceDimension(CookedShaderResourceDimension dimension) noexcept
	{
		switch (dimension)
		{
			case CookedShaderResourceDimension::Buffer: return "Buffer";
			case CookedShaderResourceDimension::Texture1D: return "Texture1D";
			case CookedShaderResourceDimension::Texture1DArray: return "Texture1DArray";
			case CookedShaderResourceDimension::Texture2D: return "Texture2D";
			case CookedShaderResourceDimension::Texture2DArray: return "Texture2DArray";
			case CookedShaderResourceDimension::Texture2DMS: return "Texture2DMS";
			case CookedShaderResourceDimension::Texture2DMSArray: return "Texture2DMSArray";
			case CookedShaderResourceDimension::Texture3D: return "Texture3D";
			case CookedShaderResourceDimension::TextureCube: return "TextureCube";
			case CookedShaderResourceDimension::TextureCubeArray: return "TextureCubeArray";
			case CookedShaderResourceDimension::Unknown:
			default: return "Unknown";
		}
	}

	const char* FormatVisibility(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::None: return "None";
			case ShaderStageVisibility::Vertex: return "Vertex";
			case ShaderStageVisibility::Pixel: return "Pixel";
			case ShaderStageVisibility::Compute: return "Compute";
			case ShaderStageVisibility::AllGraphics: return "AllGraphics";
			case ShaderStageVisibility::All: return "All";
			default: return "Unknown";
		}
	}

	const char* FormatSemanticKind(ShaderParameterSemanticKind kind) noexcept
	{
		switch (kind)
		{
			case ShaderParameterSemanticKind::ReadTexture: return "ReadTexture";
			case ShaderParameterSemanticKind::ReadBuffer: return "ReadBuffer";
			case ShaderParameterSemanticKind::RWTexture: return "RWTexture";
			case ShaderParameterSemanticKind::RWBuffer: return "RWBuffer";
			case ShaderParameterSemanticKind::RenderTarget: return "RenderTarget";
			case ShaderParameterSemanticKind::DepthTarget: return "DepthTarget";
			case ShaderParameterSemanticKind::UniformData: return "UniformData";
			case ShaderParameterSemanticKind::SamplerSet: return "SamplerSet";
			case ShaderParameterSemanticKind::AccelerationStructure: return "AccelerationStructure";
			default: return "Unknown";
		}
	}

	std::string FormatExpectedParameters(const std::vector<PassParameterDesc>& expectedParameters)
	{
		if (expectedParameters.empty())
		{
			return "<none>";
		}

		std::string result;
		for (const PassParameterDesc& parameter : expectedParameters)
		{
			result += result.empty() ? "" : "; ";
			result += std::format(
			    "{}(shader='{}', kind={}, visibility={}, size={}, array={})",
			    parameter.Name,
			    parameter.GetShaderName(),
			    FormatSemanticKind(parameter.Kind),
			    FormatVisibility(parameter.Visibility),
			    parameter.ValueSizeInBytes,
			    parameter.ArrayCount);
		}
		return result;
	}

	void AppendReflectedBinding(
	    std::string& result,
	    const LoadedShaderPackage& package,
	    const CookedShaderBinaryRecord& binaryRecord,
	    const CookedShaderResourceBindingRecord& resourceBinding)
	{
		const std::string_view resourceName =
		    package.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
		result += result.empty() ? "" : "; ";
		result += std::format(
		    "{}:{}('{}', dim={}, space={}, slot={}, array={}, size={})",
		    GetShaderStagePrefix(binaryRecord.Stage),
		    FormatResourceKindName(resourceBinding.Kind),
		    resourceName.empty() ? std::string_view{"<invalid>"} : resourceName,
		    FormatResourceDimension(resourceBinding.Dimension),
		    resourceBinding.Set,
		    resourceBinding.Slot,
		    resourceBinding.ArrayCount,
		    resourceBinding.SizeInBytes);
	}

	std::string FormatReflectedBindings(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    CookedShaderBinaryFormat requiredBinaryFormat)
	{
		const auto& binaryRecords = package.GetBinaryRecords();
		const auto& reflectionRecords = package.GetReflectionRecords();
		const auto& resourceBindings = package.GetResourceBindings();

		std::string result;
		for (std::size_t reflectionIndex = 0;
		     reflectionIndex < reflectionRecords.size() && reflectionIndex < binaryRecords.size();
		     ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (!package.IsRuntimeBinary(binaryRecord, requiredBinaryFormat) ||
			    !CookedShaderBindingRules::HasAllStages(definition.ExpectedStages, ToShaderStageMask(binaryRecord.Stage)))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (bindingIndex >= resourceBindings.size())
				{
					result += result.empty() ? "" : "; ";
					result += std::format("{}:<out-of-range:{}>", GetShaderStagePrefix(binaryRecord.Stage), bindingIndex);
					continue;
				}
				AppendReflectedBinding(result, package, binaryRecord, resourceBindings[bindingIndex]);
			}
		}

		return result.empty() ? "<none>" : result;
	}
}

const char* CookedShaderBindingDiagnostics::FormatResourceKind(CookedShaderResourceKind kind) noexcept
{
	return FormatResourceKindName(kind);
}

std::string CookedShaderBindingDiagnostics::Append(
    std::string message,
    const LoadedShaderPackage& package,
    const ShaderPackageDefinition& definition,
    const std::vector<PassParameterDesc>& expectedParameters,
    CookedShaderBinaryFormat requiredBinaryFormat)
{
	message += std::format(
	    " Expected runtime bindings=[{}]. Reflected {}/{} bindings=[{}].",
	    FormatExpectedParameters(expectedParameters),
	    CookedShaderBinaryFormatToString(requiredBinaryFormat),
	    GetRuntimeShaderCodegenTarget(requiredBinaryFormat),
	    FormatReflectedBindings(package, definition, requiredBinaryFormat));
	return message;
}
