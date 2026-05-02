#include "PCH.h"

#include "Shaders/ShaderPackageLayoutBuilder.h"

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

bool ShaderPackageLayoutBuilder::Build(
    std::string_view packageId,
    std::span<const ShaderRegistrationDesc> registrations,
    PassParameterLayout& outLayout,
    std::string& outErrorMessage)
{
	if (packageId.empty())
	{
		outErrorMessage = "Shader package layout generation requires a non-empty package id.";
		return false;
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
				outErrorMessage = std::format(
				    "Shader package '{}' contains an unnamed parameter in shader '{}' parameter struct '{}'.",
				    packageId,
				    registration.ShaderName,
				    descriptor.Name);
				return false;
			}

			if (parameter.ArrayCount == 0)
			{
				outErrorMessage = std::format(
				    "Shader package '{}' parameter '{}' in shader '{}' has invalid array count 0.",
				    packageId,
				    parameter.Name,
				    registration.ShaderName);
				return false;
			}

			if (!MergeParameter(entries, std::move(parameter), field.ValueAlignmentInBytes, registration, descriptor.Name, outErrorMessage))
			{
				return false;
			}
		}
	}

	if (!foundPackage)
	{
		outErrorMessage = std::format("No shader registrations were found for package '{}'.", packageId);
		return false;
	}

	const std::string debugName(packageId);
	outLayout = PassParameterLayout(debugName.c_str());
	AppendCategory(outLayout, entries, 0u);
	AppendCategory(outLayout, entries, 1u);
	AppendCategory(outLayout, entries, 2u);

	outErrorMessage.clear();
	return true;
}

bool BuildRegisteredShaderPackageLayout(
    std::string_view packageId,
    PassParameterLayout& outLayout,
    std::string& outErrorMessage)
{
	return ShaderPackageLayoutBuilder::Build(packageId, GlobalShaderRegistry::GetRegistrations(), outLayout, outErrorMessage);
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

PassParameterDesc ShaderPackageLayoutBuilder::BuildParameterDesc(
    const ShaderParameterStructFieldDescriptor& field,
    ShaderStage stage)
{
	PassParameterDesc parameter{};
	parameter.Name = std::string(field.GetLayoutName());
	parameter.ShaderName = std::string(field.GetShaderName());
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

bool ShaderPackageLayoutBuilder::MergeParameter(
    std::vector<MergeEntry>& entries,
    PassParameterDesc parameter,
    std::uint32_t valueAlignmentInBytes,
    const ShaderRegistrationDesc& registration,
    std::string_view shaderStructName,
    std::string& outErrorMessage)
{
	const auto existing = std::ranges::find_if(
	    entries,
	    [&parameter](const MergeEntry& entry)
	    {
		    return entry.Parameter.Name == parameter.Name;
	    });

	if (existing == entries.end())
	{
		entries.push_back(MergeEntry{
		    .Parameter = std::move(parameter),
		    .ValueAlignmentInBytes = valueAlignmentInBytes,
		    .SourceShaderName = std::string(registration.ShaderName),
		    .SourceStructName = std::string(shaderStructName),
		    .SourceStage = registration.Stage,
		});
		return true;
	}

	if (!AreCompatible(*existing, parameter, valueAlignmentInBytes))
	{
		outErrorMessage = std::format(
		    "Shader package '{}' has incompatible binding '{}': first declared by shader '{}' struct '{}' as {}; shader '{}' struct '{}' declares {}.",
		    GetShaderRegistrationPackageId(registration),
		    parameter.Name,
		    existing->SourceShaderName,
		    existing->SourceStructName,
		    FormatParameterDesc(existing->Parameter, existing->ValueAlignmentInBytes),
		    registration.ShaderName,
		    shaderStructName,
		    FormatParameterDesc(parameter, valueAlignmentInBytes));
		return false;
	}

	existing->Parameter.Visibility |= parameter.Visibility;
	return true;
}

bool ShaderPackageLayoutBuilder::AreCompatible(
    const MergeEntry& existing,
    const PassParameterDesc& incoming,
    std::uint32_t incomingAlignment) noexcept
{
	const PassParameterDesc& current = existing.Parameter;
	if (current.ShaderName != incoming.ShaderName || current.Kind != incoming.Kind || current.ResourceDomain != incoming.ResourceDomain || current.Access != incoming.Access ||
	    current.ArrayCount != incoming.ArrayCount || current.ValueSizeInBytes != incoming.ValueSizeInBytes)
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
	    "shaderName='{}' kind={} domain={} access={} array={} size={} align={} visibility={}",
	    parameter.GetShaderName(),
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