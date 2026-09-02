#include "PCH.h"

#include "Verification/ShaderParameterStructVerifier.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Json/JsonWriter.h"

#include <format>
#include <sstream>

static const auto g_shaderParameterStructVerifierLogger = Logging::GetOrCreateLogger("ShaderCompiler.ParameterStructVerifier");

static const char* GetResourceKindName(CookedShaderResourceKind kind) noexcept
{
	switch (kind)
	{
		case CookedShaderResourceKind::Unknown:
			return "Unknown";
		case CookedShaderResourceKind::ConstantBuffer:
			return "ConstantBuffer";
		case CookedShaderResourceKind::Texture:
			return "Texture";
		case CookedShaderResourceKind::StructuredBuffer:
			return "StructuredBuffer";
		case CookedShaderResourceKind::ByteAddressBuffer:
			return "ByteAddressBuffer";
		case CookedShaderResourceKind::TypedBuffer:
			return "TypedBuffer";
		case CookedShaderResourceKind::RWTexture:
			return "RWTexture";
		case CookedShaderResourceKind::RWStructuredBuffer:
			return "RWStructuredBuffer";
		case CookedShaderResourceKind::RWByteAddressBuffer:
			return "RWByteAddressBuffer";
		case CookedShaderResourceKind::RWTypedBuffer:
			return "RWTypedBuffer";
		case CookedShaderResourceKind::Sampler:
			return "Sampler";
		case CookedShaderResourceKind::AccelerationStructure:
			return "AccelerationStructure";
		case CookedShaderResourceKind::PushConstantBlock:
			return "PushConstantBlock";
	}
	Diagnostics::Fatal(g_shaderParameterStructVerifierLogger, __FILE__, __LINE__, "Unknown cooked shader resource kind.");
}

static const char* GetResourceDimensionName(CookedShaderResourceDimension dimension) noexcept
{
	switch (dimension)
	{
		case CookedShaderResourceDimension::Unknown:
			return "Unknown";
		case CookedShaderResourceDimension::Buffer:
			return "Buffer";
		case CookedShaderResourceDimension::Texture1D:
			return "Texture1D";
		case CookedShaderResourceDimension::Texture1DArray:
			return "Texture1DArray";
		case CookedShaderResourceDimension::Texture2D:
			return "Texture2D";
		case CookedShaderResourceDimension::Texture2DArray:
			return "Texture2DArray";
		case CookedShaderResourceDimension::Texture2DMS:
			return "Texture2DMS";
		case CookedShaderResourceDimension::Texture2DMSArray:
			return "Texture2DMSArray";
		case CookedShaderResourceDimension::Texture3D:
			return "Texture3D";
		case CookedShaderResourceDimension::TextureCube:
			return "TextureCube";
		case CookedShaderResourceDimension::TextureCubeArray:
			return "TextureCubeArray";
	}
	Diagnostics::Fatal(g_shaderParameterStructVerifierLogger, __FILE__, __LINE__, "Unknown cooked shader resource dimension.");
}

static const ShaderReflectionResourceBinding* FindReflectionBinding(const ShaderReflection& reflection, std::string_view name) noexcept
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
	return FindReflectionBinding(reflection, field.Name);
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
	stream << "  \"status\": \"" << (mismatches.empty() ? "matched" : "mismatch") << "\",\n";
	stream << "  \"diagnostics\": [\n";
	for (std::size_t index = 0; index < diagnostics.size(); ++index)
	{
		stream << "    " << Json::QuoteString(diagnostics[index]);
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
    const ShaderReflection& reflection,
    bool allowUnreflectedDeclarations)
{
	ShaderParameterStructVerificationResult result;

	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		if (!field.Reflected)
		{
			continue;
		}

		const ShaderReflectionResourceBinding* binding = FindReflectionBinding(reflection, field);
		if (binding == nullptr)
		{
			if (allowUnreflectedDeclarations)
			{
				result.diagnostics.push_back(
				    std::format(
				        "SC2006 declared binding is not referenced by this library export: name='{}' declaredKind='{}' "
				        "declaredDimension='{}'",
				        field.Name,
				        GetResourceKindName(field.Kind),
				        GetResourceDimensionName(field.Dimension)));
				continue;
			}
			result.mismatches.push_back(
			    std::format(
			        "SC2001 missing reflected binding: name='{}' declaredKind='{}' declaredDimension='{}'",
			        field.Name,
			        GetResourceKindName(field.Kind),
			        GetResourceDimensionName(field.Dimension)));
			continue;
		}

		if (binding->Kind != field.Kind)
		{
			result.mismatches.push_back(
			    std::format(
			        "SC2002 reflected kind mismatch: name='{}' reflectedBinding='{}' declaredKind='{}' reflectedKind='{}'",
			        field.Name,
			        binding->Name,
			        GetResourceKindName(field.Kind),
			        GetResourceKindName(binding->Kind)));
		}

		if (field.Dimension != CookedShaderResourceDimension::Unknown && binding->Dimension != CookedShaderResourceDimension::Unknown
		    && binding->Dimension != field.Dimension)
		{
			result.mismatches.push_back(
			    std::format(
			        "SC2003 reflected dimension mismatch: name='{}' reflectedBinding='{}' declaredDimension='{}' reflectedDimension='{}'",
			        field.Name,
			        binding->Name,
			        GetResourceDimensionName(field.Dimension),
			        GetResourceDimensionName(binding->Dimension)));
		}

		if (binding->ArrayCount != field.ArrayCount)
		{
			result.mismatches.push_back(
			    std::format(
			        "SC2004 reflected array-count mismatch: name='{}' reflectedBinding='{}' declaredCount={} reflectedCount={}",
			        field.Name,
			        binding->Name,
			        field.ArrayCount,
			        binding->ArrayCount));
		}
	}

	for (const ShaderReflectionResourceBinding& binding : reflection.Bindings)
	{
		if (FindDescriptorField(descriptor, binding.Name) == nullptr)
		{
			result.diagnostics.push_back(
			    std::format(
			        "SC2005 extra reflected binding: reflectedBinding='{}' reflectedKind='{}' reflectedDimension='{}' is not declared in "
			        "parameter struct '{}'",
			        binding.Name,
			        GetResourceKindName(binding.Kind),
			        GetResourceDimensionName(binding.Dimension),
			        descriptor.Name));
		}
	}

	result.diagnostics.insert(result.diagnostics.begin(), result.mismatches.begin(), result.mismatches.end());
	if (result.mismatches.empty())
	{
		result.diagnostics.push_back(std::format("SC2000 parameter struct '{}' matches reflected resource bindings", descriptor.Name));
	}

	return result;
}
