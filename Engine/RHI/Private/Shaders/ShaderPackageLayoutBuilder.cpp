#include "PCH.h"

#include "Shaders/ShaderPackageLayoutBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <algorithm>
#include <format>
#include <string>
#include <utility>

struct ShaderPackageLayoutBuilder::MergeEntry final
{
	PassParameterDesc Parameter;
	std::uint32_t ValueAlignmentInBytes = 0;
	std::string SourceShaderName;
	std::string SourceStructName;
	ShaderStage SourceStage = ShaderStage::Count;
};

PassParameterLayout ShaderPackageLayoutBuilder::Build(std::string_view packageId, std::span<const ShaderRegistrationDesc> registrations)
{
	if (packageId.empty())
	{
		throw Diagnostics::Error("Shader package layout generation received an empty package identity.");
	}

	std::vector<MergeEntry> entries;
	bool foundPackage = false;
	for (const ShaderRegistrationDesc& registration : registrations)
	{
		if (GetShaderRegistrationPackageId(registration) != packageId)
		{
			continue;
		}

		foundPackage = true;
		if (registration.BuildParameterStructDescriptor == nullptr)
		{
			continue;
		}

		const ShaderParameterStructDescriptor descriptor = registration.BuildParameterStructDescriptor();
		for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
		{
			PassParameterDesc parameter = BuildParameterDesc(field, registration.Stage);
			if (parameter.Name.empty())
			{
				throw Diagnostics::Error(
				    std::format(
				        "Shader package '{}' contains an unnamed parameter in shader '{}' parameter struct '{}'.",
				        packageId,
				        registration.ShaderName,
				        descriptor.Name));
			}

			if (parameter.ArrayCount == 0)
			{
				throw Diagnostics::Error(
				    std::format(
				        "Shader package '{}' parameter '{}' in shader '{}' has invalid array count 0.",
				        packageId,
				        parameter.Name,
				        registration.ShaderName));
			}

			MergeParameter(entries, std::move(parameter), field.ValueAlignmentInBytes, registration, descriptor.Name);
		}
	}

	if (!foundPackage)
	{
		throw Diagnostics::Error(std::format("No shader registrations were found for package '{}'.", packageId));
	}

	const std::string debugName(packageId);
	PassParameterLayout layout(debugName.c_str());
	AppendCategory(layout, entries, 0u);
	AppendCategory(layout, entries, 1u);
	AppendCategory(layout, entries, 2u);
	return layout;
}

PassParameterLayout BuildRegisteredShaderPackageLayout(std::string_view packageId)
{
	return ShaderPackageLayoutBuilder::Build(packageId, GlobalShaderRegistry::GetRegistrations());
}

ShaderStageVisibility ShaderPackageLayoutBuilder::GetDefaultVisibility(ShaderStage stage) noexcept
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return ShaderStageVisibility::Vertex;
		case ShaderStage::Pixel:
			return ShaderStageVisibility::Pixel;
		case ShaderStage::Compute:
			return ShaderStageVisibility::Compute;
		case ShaderStage::Geometry:
		case ShaderStage::Hull:
		case ShaderStage::Domain:
			return ShaderStageVisibility::AllGraphics;
		case ShaderStage::Count:
		default:
			return ShaderStageVisibility::None;
	}
}

ShaderStageVisibility ShaderPackageLayoutBuilder::ResolveVisibility(
    const ShaderParameterStructFieldDescriptor& field,
    ShaderStage stage) noexcept
{
	return field.Visibility == ShaderStageVisibility::None ? GetDefaultVisibility(stage) : field.Visibility;
}

PassParameterDesc ShaderPackageLayoutBuilder::BuildParameterDesc(const ShaderParameterStructFieldDescriptor& field, ShaderStage stage)
{
	PassParameterDesc parameter{};
	parameter.Name = field.Name;
	parameter.Kind = field.SemanticKind;
	parameter.ResourceDomain = field.ResourceDomain;
	parameter.Access = field.Access;
	parameter.Visibility = ResolveVisibility(field, stage);
	parameter.ArrayCount = field.ArrayCount;
	parameter.ValueSizeInBytes = field.ValueSizeInBytes;
	return parameter;
}

std::uint32_t ShaderPackageLayoutBuilder::GetOrderingCategory(const PassParameterDesc& parameter) noexcept
{
	if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget || parameter.Kind == ShaderParameterSemanticKind::DepthTarget)
	{
		return 0u;
	}

	if (parameter.Kind == ShaderParameterSemanticKind::SamplerSet)
	{
		return 2u;
	}

	return 1u;
}

void ShaderPackageLayoutBuilder::MergeParameter(
    std::vector<MergeEntry>& entries,
    PassParameterDesc parameter,
    std::uint32_t valueAlignmentInBytes,
    const ShaderRegistrationDesc& registration,
    std::string_view shaderStructName)
{
	const auto existing =
	    std::ranges::find_if(entries, [&parameter](const MergeEntry& entry) { return entry.Parameter.Name == parameter.Name; });

	if (existing == entries.end())
	{
		entries.push_back(
		    MergeEntry{
		        .Parameter = std::move(parameter),
		        .ValueAlignmentInBytes = valueAlignmentInBytes,
		        .SourceShaderName = std::string(registration.ShaderName),
		        .SourceStructName = std::string(shaderStructName),
		        .SourceStage = registration.Stage,
		    });
		return;
	}

	if (!MatchesExistingBinding(*existing, parameter, valueAlignmentInBytes))
	{
		throw Diagnostics::Error(
		    std::format(
		        "Shader package '{}' has conflicting binding '{}': first declared by shader '{}' struct '{}' as {}; shader '{}' struct "
		        "'{}' "
		        "declares {}.",
		        GetShaderRegistrationPackageId(registration),
		        parameter.Name,
		        existing->SourceShaderName,
		        existing->SourceStructName,
		        FormatParameterDesc(existing->Parameter, existing->ValueAlignmentInBytes),
		        registration.ShaderName,
		        shaderStructName,
		        FormatParameterDesc(parameter, valueAlignmentInBytes)));
	}

	existing->Parameter.Visibility |= parameter.Visibility;
}

bool ShaderPackageLayoutBuilder::MatchesExistingBinding(
    const MergeEntry& existing,
    const PassParameterDesc& incoming,
    std::uint32_t incomingAlignment) noexcept
{
	const PassParameterDesc& current = existing.Parameter;
	if (current.Kind != incoming.Kind || current.ResourceDomain != incoming.ResourceDomain || current.Access != incoming.Access
	    || current.ArrayCount != incoming.ArrayCount || current.ValueSizeInBytes != incoming.ValueSizeInBytes)
	{
		return false;
	}

	if (current.Kind == ShaderParameterSemanticKind::UniformData && existing.ValueAlignmentInBytes != incomingAlignment)
	{
		return false;
	}

	return true;
}

std::string ShaderPackageLayoutBuilder::FormatParameterDesc(const PassParameterDesc& parameter, std::uint32_t alignmentInBytes)
{
	return std::format(
	    "name='{}' kind={} domain={} access={} array={} size={} align={} visibility={}",
	    parameter.Name,
	    static_cast<std::uint32_t>(parameter.Kind),
	    static_cast<std::uint32_t>(parameter.ResourceDomain),
	    static_cast<std::uint32_t>(parameter.Access),
	    parameter.ArrayCount,
	    parameter.ValueSizeInBytes,
	    alignmentInBytes,
	    static_cast<std::uint32_t>(parameter.Visibility));
}

void ShaderPackageLayoutBuilder::AppendCategory(PassParameterLayout& layout, const std::vector<MergeEntry>& entries, std::uint32_t category)
{
	for (const MergeEntry& entry : entries)
	{
		if (GetOrderingCategory(entry.Parameter) == category)
		{
			layout.AddParameter(entry.Parameter);
		}
	}
}
