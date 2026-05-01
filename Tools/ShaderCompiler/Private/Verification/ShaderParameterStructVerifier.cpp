#include "PCH.h"

#include "Verification/ShaderParameterStructVerifier.h"

#include <format>
#include <sstream>

static const char* GetResourceKindName(CookedShaderResourceKind kind) noexcept
{
	switch (kind)
	{
		case CookedShaderResourceKind::Unknown: return "Unknown";
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
	}
	return "Unknown";
}

static const char* GetResourceDimensionName(CookedShaderResourceDimension dimension) noexcept
{
	switch (dimension)
	{
		case CookedShaderResourceDimension::Unknown: return "Unknown";
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
	}
	return "Unknown";
}

static std::string EscapeJsonString(std::string_view value)
{
	std::string result;
	result.reserve(value.size() + 8);
	for (const char ch : value)
	{
		switch (ch)
		{
			case '\\':
				result += "\\\\";
				break;
			case '"':
				result += "\\\"";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				result.push_back(ch);
				break;
		}
	}
	return result;
}

static const ShaderReflectionResourceBinding* FindReflectionBinding(
	const ShaderReflection& reflection,
	std::string_view name) noexcept
{
	for (const ShaderReflectionResourceBinding& binding : reflection.Bindings)
	{
		if (binding.Name == name)
		{
			return &binding;
		}
	}
	return nullptr;
}

static const ShaderReflectionResourceBinding* FindReflectionBinding(
	const ShaderReflection& reflection,
	const ShaderParameterStructFieldDescriptor& field) noexcept
{
	if (const ShaderReflectionResourceBinding* binding = FindReflectionBinding(reflection, field.GetShaderName()))
	{
		return binding;
	}

	const std::string_view layoutName = field.GetLayoutName();
	if (layoutName != field.GetShaderName())
	{
		if (const ShaderReflectionResourceBinding* binding = FindReflectionBinding(reflection, layoutName))
		{
			return binding;
		}
	}

	return nullptr;
}

static const ShaderParameterStructFieldDescriptor* FindDescriptorField(
	const ShaderParameterStructDescriptor& descriptor,
	std::string_view name) noexcept
{
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		if (!field.Reflected)
		{
			continue;
		}

		if (field.GetShaderName() == name || field.GetLayoutName() == name)
		{
			return &field;
		}
	}
	return nullptr;
}

std::string ShaderParameterStructVerificationResult::BuildJsonReport() const
{
	std::ostringstream stream;
	stream << "{\n";
	stream << "  \"status\": \"" << (succeeded ? "matched" : "mismatch") << "\",\n";
	stream << "  \"diagnostics\": [\n";
	for (std::size_t index = 0; index < diagnostics.size(); ++index)
	{
		stream << "    \"" << EscapeJsonString(diagnostics[index]) << "\"";
		if (index + 1 < diagnostics.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ]\n";
	stream << "}\n";
	return stream.str();
}

ShaderParameterStructVerificationResult ShaderParameterStructVerifier::Verify(
	const ShaderParameterStructDescriptor& descriptor,
	const ShaderReflection& reflection)
{
	ShaderParameterStructVerificationResult result;

	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		if (!field.Reflected)
		{
			continue;
		}

		const std::string_view shaderName = field.GetShaderName();
		const std::string_view layoutName = field.GetLayoutName();
		const ShaderReflectionResourceBinding* binding = FindReflectionBinding(reflection, field);
		if (binding == nullptr)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2001 missing reflected binding: field='{}' layout='{}' shaderBinding='{}' declaredKind='{}' declaredDimension='{}'",
			    field.Name,
			    layoutName,
			    shaderName,
			    GetResourceKindName(field.Kind),
			    GetResourceDimensionName(field.Dimension)));
			continue;
		}

		if (binding->Kind != field.Kind)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2002 reflected kind mismatch: field='{}' layout='{}' shaderBinding='{}' reflectedBinding='{}' declaredKind='{}' reflectedKind='{}'",
			    field.Name,
			    layoutName,
			    shaderName,
			    binding->Name,
			    GetResourceKindName(field.Kind),
			    GetResourceKindName(binding->Kind)));
		}

		if (field.Dimension != CookedShaderResourceDimension::Unknown && binding->Dimension != CookedShaderResourceDimension::Unknown &&
		    binding->Dimension != field.Dimension)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2003 reflected dimension mismatch: field='{}' layout='{}' shaderBinding='{}' reflectedBinding='{}' declaredDimension='{}' reflectedDimension='{}'",
			    field.Name,
			    layoutName,
			    shaderName,
			    binding->Name,
			    GetResourceDimensionName(field.Dimension),
			    GetResourceDimensionName(binding->Dimension)));
		}

		if (binding->ArrayCount != field.ArrayCount)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2004 reflected array-count mismatch: field='{}' layout='{}' shaderBinding='{}' reflectedBinding='{}' declaredCount={} reflectedCount={}",
			    field.Name,
			    layoutName,
			    shaderName,
			    binding->Name,
			    field.ArrayCount,
			    binding->ArrayCount));
		}
	}

	for (const ShaderReflectionResourceBinding& binding : reflection.Bindings)
	{
		if (FindDescriptorField(descriptor, binding.Name) == nullptr)
		{
			result.diagnostics.push_back(std::format(
			    "SC2005 extra reflected binding: reflectedBinding='{}' reflectedKind='{}' reflectedDimension='{}' is not declared in parameter struct '{}'",
			    binding.Name,
			    GetResourceKindName(binding.Kind),
			    GetResourceDimensionName(binding.Dimension),
			    descriptor.Name));
		}
	}

	if (result.succeeded)
	{
		result.diagnostics.push_back(std::format("SC2000 parameter struct '{}' matches reflected resource bindings", descriptor.Name));
	}

	return result;
}