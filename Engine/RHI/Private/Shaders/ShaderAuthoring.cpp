#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <format>
#include <sstream>
#include <vector>

void RegisterBuiltinGlobalShaders() noexcept;

static void EnsureGlobalShaderRegistrationBootstrap() noexcept
{
	static const bool initialized = []
	{
		RegisterBuiltinGlobalShaders();
		return true;
	}();
	(void)initialized;
}

static std::vector<ShaderRegistrationDesc>& MutableGlobalShaderRegistrations()
{
	static std::vector<ShaderRegistrationDesc> registrations;
	return registrations;
}

static std::vector<RayTracingHitGroupRegistrationDesc>& MutableRayTracingHitGroupRegistrations()
{
	static std::vector<RayTracingHitGroupRegistrationDesc> registrations;
	return registrations;
}

void GlobalShaderRegistry::Register(ShaderRegistrationDesc desc)
{
	EnsureGlobalShaderRegistrationBootstrap();

	std::vector<ShaderRegistrationDesc>& registrations = MutableGlobalShaderRegistrations();
	const auto existing = std::ranges::find_if(
	    registrations,
	    [shaderName = desc.ShaderName](const ShaderRegistrationDesc& registeredDesc)
	    {
		    return registeredDesc.ShaderName == shaderName;
	    });
	if (existing != registrations.end())
	{
		return;
	}

	registrations.push_back(desc);
}

void GlobalShaderRegistry::RegisterRayTracingHitGroup(RayTracingHitGroupRegistrationDesc desc)
{
	EnsureGlobalShaderRegistrationBootstrap();

	std::vector<RayTracingHitGroupRegistrationDesc>& registrations = MutableRayTracingHitGroupRegistrations();
	const auto existing = std::ranges::find_if(
	    registrations,
	    [desc](const RayTracingHitGroupRegistrationDesc& registeredDesc)
	    {
		    return registeredDesc.PackageName == desc.PackageName && registeredDesc.HitGroupName == desc.HitGroupName;
	    });
	if (existing != registrations.end())
	{
		return;
	}

	registrations.push_back(desc);
}

std::span<const ShaderRegistrationDesc> GlobalShaderRegistry::GetRegistrations() noexcept
{
	EnsureGlobalShaderRegistrationBootstrap();
	return MutableGlobalShaderRegistrations();
}

std::span<const RayTracingHitGroupRegistrationDesc> GlobalShaderRegistry::GetRayTracingHitGroups() noexcept
{
	EnsureGlobalShaderRegistrationBootstrap();
	return MutableRayTracingHitGroupRegistrations();
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindByName(std::string_view shaderName) noexcept
{
	EnsureGlobalShaderRegistrationBootstrap();

	const std::vector<ShaderRegistrationDesc>& registrations = MutableGlobalShaderRegistrations();
	const auto found = std::ranges::find_if(
	    registrations,
	    [shaderName](const ShaderRegistrationDesc& desc)
	    {
		    return desc.ShaderName == shaderName;
	    });
	return found != registrations.end() ? &(*found) : nullptr;
}

ShaderPermutationKey BuildShaderPermutationKey(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector) noexcept
{
	if (domain.Dimensions.empty() || IsDefaultShaderPermutationVector(vector))
	{
		return 0;
	}

	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	const std::uint64_t dimensionCount = static_cast<std::uint64_t>(domain.Dimensions.size());
	hash = Hash::ContinueFnv1a64Value(hash, dimensionCount);
	for (std::size_t index = 0; index < domain.Dimensions.size(); ++index)
	{
		const ShaderPermutationDimensionDescriptor& dimension = domain.Dimensions[index];
		hash = Hash::ContinueFnv1a64(hash, dimension.Name.data(), dimension.Name.size());
		hash = Hash::ContinueFnv1a64(hash, dimension.DefineName.data(), dimension.DefineName.size());
		hash = Hash::ContinueFnv1a64Value(hash, dimension.ValueCount);
		for (const ShaderPermutationValueDescriptor& valueDesc : dimension.Values)
		{
			hash = Hash::ContinueFnv1a64(hash, valueDesc.Name.data(), valueDesc.Name.size());
			hash = Hash::ContinueFnv1a64(hash, valueDesc.DefineValue.data(), valueDesc.DefineValue.size());
		}
		const std::uint32_t value = index < vector.Values.size() ? vector.Values[index] : 0u;
		hash = Hash::ContinueFnv1a64Value(hash, value);
	}
	return hash;
}

bool IsDefaultShaderPermutationVector(const ShaderPermutationVector& vector) noexcept
{
	return std::ranges::all_of(vector.Values, [](std::uint32_t value) { return value == 0; });
}

std::vector<ShaderPermutationVector> EnumerateShaderPermutationVectors(const ShaderPermutationDomainDescriptor& domain)
{
	if (domain.Dimensions.empty())
	{
		return {ShaderPermutationVector{}};
	}

	std::vector<ShaderPermutationVector> vectors;
	ShaderPermutationVector current;
	current.Values.resize(domain.Dimensions.size());

	auto enumerateRecursive = [&](auto&& self, std::size_t dimensionIndex) -> void
	{
		if (dimensionIndex >= domain.Dimensions.size())
		{
			vectors.push_back(current);
			return;
		}

		const std::uint32_t valueCount = std::max(1u, domain.Dimensions[dimensionIndex].ValueCount);
		for (std::uint32_t value = 0; value < valueCount; ++value)
		{
			current.Values[dimensionIndex] = value;
			self(self, dimensionIndex + 1);
		}
	};
	enumerateRecursive(enumerateRecursive, 0);
	return vectors;
}

std::vector<std::string> BuildShaderPermutationDefines(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector)
{
	std::vector<std::string> defines;
	defines.reserve(domain.Dimensions.size());
	for (std::size_t index = 0; index < domain.Dimensions.size(); ++index)
	{
		const ShaderPermutationDimensionDescriptor& dimension = domain.Dimensions[index];
		if (dimension.DefineName.empty())
		{
			continue;
		}

		const std::uint32_t value = index < vector.Values.size() ? vector.Values[index] : 0u;
		std::string defineValue = std::to_string(value);
		if (value < dimension.Values.size() && !dimension.Values[value].DefineValue.empty())
		{
			defineValue = dimension.Values[value].DefineValue;
		}
		defines.push_back(dimension.DefineName + "=" + defineValue);
	}
	return defines;
}

std::string BuildShaderPermutationVariantId(
	ShaderPermutationKey permutationKey)
{
	return permutationKey == 0 ? std::string("Default") : "Perm_" + Formatting::FormatHexUInt64(permutationKey);
}

std::string BuildShaderPermutationVariantId(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector)
{
	return BuildShaderPermutationVariantId(BuildShaderPermutationKey(domain, vector));
}

std::string BuildShaderPermutationVectorName(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector)
{
	if (domain.Dimensions.empty())
	{
		return "Default";
	}

	std::ostringstream stream;
	for (std::size_t index = 0; index < domain.Dimensions.size(); ++index)
	{
		if (index > 0)
		{
			stream << ", ";
		}

		const ShaderPermutationDimensionDescriptor& dimension = domain.Dimensions[index];
		const std::uint32_t value = index < vector.Values.size() ? vector.Values[index] : 0u;
		stream << dimension.Name << '=' << value;
		if (value < dimension.Values.size() && !dimension.Values[value].Name.empty())
		{
			stream << '(' << dimension.Values[value].Name << ')';
		}
	}
	return stream.str();
}

ShaderPermutationDimensionDescriptor MakeShaderPermutationBoolDimension(
	std::string_view name,
	std::string_view defineName)
{
	return ShaderPermutationDimensionDescriptor{
	    .Name = std::string(name),
	    .DefineName = std::string(defineName),
	    .ValueCount = 2,
	    .Values = {{.Name = "false", .DefineValue = "0"}, {.Name = "true", .DefineValue = "1"}}};
}

ShaderPermutationDimensionDescriptor MakeShaderPermutationEnumDimension(
	std::string_view name,
	std::string_view defineName,
	std::initializer_list<std::string_view> values)
{
	ShaderPermutationDimensionDescriptor dimension;
	dimension.Name = std::string(name);
	dimension.DefineName = std::string(defineName);
	dimension.ValueCount = static_cast<std::uint32_t>(values.size());
	dimension.Values.reserve(values.size());
	std::uint32_t valueIndex = 0;
	for (std::string_view value : values)
	{
		dimension.Values.push_back(ShaderPermutationValueDescriptor{
		    .Name = std::string(value),
		    .DefineValue = std::to_string(valueIndex)});
		++valueIndex;
	}
	return dimension;
}

std::uint32_t GetShaderPermutationValue(
	const ShaderPermutationDomainDescriptor& domain,
	const ShaderPermutationVector& vector,
	std::string_view dimensionName) noexcept
{
	for (std::size_t dimensionIndex = 0; dimensionIndex < domain.Dimensions.size(); ++dimensionIndex)
	{
		if (domain.Dimensions[dimensionIndex].Name == dimensionName)
		{
			return dimensionIndex < vector.Values.size() ? vector.Values[dimensionIndex] : 0u;
		}
	}
	return 0;
}

std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor)
{
	std::ostringstream stream;
	stream << descriptor.Name << " parameter(s)=" << descriptor.Fields.size();
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		stream << "\n  layout=" << field.GetLayoutName() << " shader=" << field.GetShaderName()
		       << " kind=" << static_cast<std::uint32_t>(field.Kind)
		       << " dimension=" << static_cast<std::uint32_t>(field.Dimension)
		       << " semantic=" << static_cast<std::uint32_t>(field.SemanticKind)
		       << " domain=" << static_cast<std::uint32_t>(field.ResourceDomain)
		       << " access=" << static_cast<std::uint32_t>(field.Access)
		       << " visibility=" << static_cast<std::uint32_t>(field.Visibility)
		       << " array=" << field.ArrayCount;
		if (field.ValueSizeInBytes > 0)
		{
			stream << " size=" << field.ValueSizeInBytes;
		}
		stream << " reflected=" << (field.Reflected ? "true" : "false");
	}
	return stream.str();
}