#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <algorithm>
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

static std::uint64_t HashBytes(std::uint64_t hash, const void* data, std::size_t size) noexcept
{
	const auto* bytes = static_cast<const std::uint8_t*>(data);
	for (std::size_t index = 0; index < size; ++index)
	{
		hash ^= static_cast<std::uint64_t>(bytes[index]);
		hash *= 1099511628211ull;
	}
	return hash;
}

static std::uint64_t HashString(std::uint64_t hash, std::string_view value) noexcept
{
	return HashBytes(hash, value.data(), value.size());
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

std::span<const ShaderRegistrationDesc> GlobalShaderRegistry::GetRegistrations() noexcept
{
	EnsureGlobalShaderRegistrationBootstrap();
	return MutableGlobalShaderRegistrations();
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
	std::uint64_t hash = 14695981039346656037ull;
	const std::uint64_t dimensionCount = static_cast<std::uint64_t>(domain.Dimensions.size());
	hash = HashBytes(hash, &dimensionCount, sizeof(dimensionCount));
	for (std::size_t index = 0; index < domain.Dimensions.size(); ++index)
	{
		const ShaderPermutationDimensionDescriptor& dimension = domain.Dimensions[index];
		hash = HashString(hash, dimension.Name);
		hash = HashBytes(hash, &dimension.ValueCount, sizeof(dimension.ValueCount));
		const std::uint32_t value = index < vector.Values.size() ? vector.Values[index] : 0u;
		hash = HashBytes(hash, &value, sizeof(value));
	}
	return hash;
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