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
	(void) initialized;
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

std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor)
{
	std::ostringstream stream;
	stream << descriptor.Name << " parameter(s)=" << descriptor.Fields.size();
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		stream << "\n  layout=" << field.GetLayoutName() << " shader=" << field.GetShaderName()
		       << " kind=" << static_cast<std::uint32_t>(field.Kind) << " dimension=" << static_cast<std::uint32_t>(field.Dimension)
		       << " semantic=" << static_cast<std::uint32_t>(field.SemanticKind)
		       << " domain=" << static_cast<std::uint32_t>(field.ResourceDomain) << " access=" << static_cast<std::uint32_t>(field.Access)
		       << " visibility=" << static_cast<std::uint32_t>(field.Visibility) << " array=" << field.ArrayCount;
		if (field.ValueSizeInBytes > 0)
		{
			stream << " size=" << field.ValueSizeInBytes;
		}
		stream << " reflected=" << (field.Reflected ? "true" : "false");
	}
	return stream.str();
}