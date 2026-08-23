#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <algorithm>
#include <sstream>
#include <vector>

std::string BuildShaderPackageIdFromSourcePath(std::string_view sourcePath)
{
	const std::size_t slash = sourcePath.find_last_of("/\\");
	const std::string_view fileName = sourcePath.substr(slash == std::string_view::npos ? 0 : slash + 1);
	const std::size_t dot = fileName.find_last_of('.');
	return std::string(dot != std::string_view::npos && dot > 0 ? fileName.substr(0, dot) : fileName);
}

std::string GetShaderRegistrationPackageId(const ShaderRegistrationDesc& shader)
{
	if (!shader.PackageName.empty())
	{
		return std::string(shader.PackageName);
	}

	std::string packageId = BuildShaderPackageIdFromSourcePath(shader.SourcePath);
	return packageId.empty() ? std::string(shader.ShaderName) : packageId;
}

std::string GetShaderRegistrationBindingLayoutId(const ShaderRegistrationDesc& shader)
{
	return shader.BindingLayoutId.empty() ? GetShaderRegistrationPackageId(shader) : std::string(shader.BindingLayoutId);
}

CookedShaderPackageKind GetDefaultCookedShaderPackageKind(ShaderStage stage) noexcept
{
	return stage == ShaderStage::Compute ? CookedShaderPackageKind::Compute : CookedShaderPackageKind::Graphics;
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

static const std::vector<ShaderRegistrationDesc>& GlobalShaderRegistrationSnapshot() noexcept
{
	static const std::vector<ShaderRegistrationDesc> registrations = []
	{
		std::vector<ShaderRegistrationDesc> snapshot = MutableGlobalShaderRegistrations();
		std::ranges::sort(
		    snapshot,
		    [](const ShaderRegistrationDesc& left, const ShaderRegistrationDesc& right)
		    {
			    if (left.PackageName != right.PackageName)
			    {
				    return left.PackageName < right.PackageName;
			    }
			    if (left.ShaderName != right.ShaderName)
			    {
				    return left.ShaderName < right.ShaderName;
			    }
			    if (left.EntryPoint != right.EntryPoint)
			    {
				    return left.EntryPoint < right.EntryPoint;
			    }
			    return static_cast<std::uint32_t>(left.Stage) < static_cast<std::uint32_t>(right.Stage);
		    });
		return snapshot;
	}();
	return registrations;
}

static const std::vector<RayTracingHitGroupRegistrationDesc>& RayTracingHitGroupRegistrationSnapshot() noexcept
{
	static const std::vector<RayTracingHitGroupRegistrationDesc> registrations = []
	{
		std::vector<RayTracingHitGroupRegistrationDesc> snapshot = MutableRayTracingHitGroupRegistrations();
		std::ranges::sort(
		    snapshot,
		    [](const RayTracingHitGroupRegistrationDesc& left, const RayTracingHitGroupRegistrationDesc& right)
		    {
			    if (left.PackageName != right.PackageName)
			    {
				    return left.PackageName < right.PackageName;
			    }
			    return left.HitGroupName < right.HitGroupName;
		    });
		return snapshot;
	}();
	return registrations;
}

void GlobalShaderRegistry::Register(ShaderRegistrationDesc desc)
{
	std::vector<ShaderRegistrationDesc>& registrations = MutableGlobalShaderRegistrations();
	const auto existing = std::ranges::find_if(
	    registrations,
	    [shaderName = desc.ShaderName](const ShaderRegistrationDesc& registeredDesc) { return registeredDesc.ShaderName == shaderName; });
	if (existing != registrations.end())
	{
		return;
	}

	registrations.push_back(desc);
}

void GlobalShaderRegistry::RegisterRayTracingHitGroup(RayTracingHitGroupRegistrationDesc desc)
{
	std::vector<RayTracingHitGroupRegistrationDesc>& registrations = MutableRayTracingHitGroupRegistrations();
	const auto existing = std::ranges::find_if(
	    registrations,
	    [desc](const RayTracingHitGroupRegistrationDesc& registeredDesc)
	    { return registeredDesc.PackageName == desc.PackageName && registeredDesc.HitGroupName == desc.HitGroupName; });
	if (existing != registrations.end())
	{
		return;
	}

	registrations.push_back(desc);
}

std::span<const ShaderRegistrationDesc> GlobalShaderRegistry::GetRegistrations() noexcept
{
	return GlobalShaderRegistrationSnapshot();
}

std::span<const RayTracingHitGroupRegistrationDesc> GlobalShaderRegistry::GetRayTracingHitGroups() noexcept
{
	return RayTracingHitGroupRegistrationSnapshot();
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindByName(std::string_view shaderName) noexcept
{
	const std::vector<ShaderRegistrationDesc>& registrations = GlobalShaderRegistrationSnapshot();
	const auto found =
	    std::ranges::find_if(registrations, [shaderName](const ShaderRegistrationDesc& desc) { return desc.ShaderName == shaderName; });
	return found != registrations.end() ? &(*found) : nullptr;
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindByType(const std::type_info& shaderType) noexcept
{
	const std::span<const ShaderRegistrationDesc> registrations = GetRegistrations();
	const auto registration = std::ranges::find_if(
	    registrations,
	    [&shaderType](const ShaderRegistrationDesc& candidate)
	    { return candidate.ShaderType != nullptr && *candidate.ShaderType == shaderType; });
	return registration != registrations.end() ? &*registration : nullptr;
}

std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor)
{
	std::ostringstream stream;
	stream << descriptor.Name << " parameter(s)=" << descriptor.Fields.size();
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		stream << "\n  name=" << field.Name << " kind=" << static_cast<std::uint32_t>(field.Kind)
		       << " dimension=" << static_cast<std::uint32_t>(field.Dimension)
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
