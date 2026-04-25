#include "PCH.h"

#include "Verification/ShaderParameterStructVerifier.h"

#include <format>
#include <sstream>

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

static const ShaderParameterStructFieldDescriptor* FindDescriptorField(
	const ShaderParameterStructDescriptor& descriptor,
	std::string_view name) noexcept
{
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		if (field.Name == name)
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
		const ShaderReflectionResourceBinding* binding = FindReflectionBinding(reflection, field.Name);
		if (binding == nullptr)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format("SC2001 missing reflected binding for parameter '{}'", field.Name));
			continue;
		}

		if (binding->Kind != field.Kind)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2002 kind mismatch for parameter '{}' (declared={}, reflected={})",
			    field.Name,
			    static_cast<std::uint32_t>(field.Kind),
			    static_cast<std::uint32_t>(binding->Kind)));
		}

		if (field.Dimension != CookedShaderResourceDimension::Unknown && binding->Dimension != CookedShaderResourceDimension::Unknown &&
		    binding->Dimension != field.Dimension)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2003 dimension mismatch for parameter '{}' (declared={}, reflected={})",
			    field.Name,
			    static_cast<std::uint32_t>(field.Dimension),
			    static_cast<std::uint32_t>(binding->Dimension)));
		}

		if (binding->ArrayCount != field.ArrayCount)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format(
			    "SC2004 array-count mismatch for parameter '{}' (declared={}, reflected={})",
			    field.Name,
			    field.ArrayCount,
			    binding->ArrayCount));
		}
	}

	for (const ShaderReflectionResourceBinding& binding : reflection.Bindings)
	{
		if (FindDescriptorField(descriptor, binding.Name) == nullptr)
		{
			result.succeeded = false;
			result.diagnostics.push_back(std::format("SC2005 extra reflected binding '{}' is not declared in parameter struct", binding.Name));
		}
	}

	if (result.succeeded)
	{
		result.diagnostics.push_back(std::format("SC2000 parameter struct '{}' matches reflected resource bindings", descriptor.Name));
	}

	return result;
}