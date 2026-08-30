#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include "Core/Public/Hash/HashUtils.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <algorithm>
#include <string>
#include <vector>

ShaderTypeId BuildShaderTypeId(std::string_view shaderName) noexcept
{
	constexpr std::string_view shaderTypeDomain = "Sparkle.ShaderType|";
	std::uint64_t hash = Hash::ContinueFnv1a64(Hash::kFnv64OffsetBasis, shaderTypeDomain.data(), shaderTypeDomain.size());
	hash = Hash::ContinueFnv1a64(hash, shaderName.data(), shaderName.size());
	return Hash::FinalizeFnv1a64(hash);
}

ShaderParameterSignature BuildShaderParameterSignature(const PassParameterLayout& layout) noexcept
{
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	for (const PassParameterDesc& parameter : layout.GetParameters())
	{
		hash = Hash::ContinueFnv1a64(hash, parameter.Name.data(), parameter.Name.size());
		hash = Hash::ContinueFnv1a64Value(hash, parameter.Kind);
		hash = Hash::ContinueFnv1a64Value(hash, parameter.ResourceDomain);
		hash = Hash::ContinueFnv1a64Value(hash, parameter.Access);
		hash = Hash::ContinueFnv1a64Value(hash, parameter.Visibility);
		hash = Hash::ContinueFnv1a64Value(hash, parameter.ArrayCount);
		hash = Hash::ContinueFnv1a64Value(hash, parameter.ValueSizeInBytes);
	}
	return Hash::FinalizeFnv1a64(hash);
}

static std::vector<ShaderRegistrationDesc>& MutableGlobalShaderRegistrations()
{
	static std::vector<ShaderRegistrationDesc> registrations;
	return registrations;
}

static const std::vector<ShaderRegistrationDesc>& GlobalShaderRegistrationSnapshot() noexcept
{
	static const std::vector<ShaderRegistrationDesc> registrations = []
	{
		std::vector<ShaderRegistrationDesc> snapshot = MutableGlobalShaderRegistrations();
		std::ranges::sort(
		    snapshot,
		    [](const ShaderRegistrationDesc& left, const ShaderRegistrationDesc& right) { return left.TypeId < right.TypeId; });
		return snapshot;
	}();
	return registrations;
}

void GlobalShaderRegistry::Register(ShaderRegistrationDesc desc)
{
	MutableGlobalShaderRegistrations().push_back(desc);
}

std::span<const ShaderRegistrationDesc> GlobalShaderRegistry::GetRegistrations() noexcept
{
	return GlobalShaderRegistrationSnapshot();
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindByName(std::string_view shaderName) noexcept
{
	const std::span<const ShaderRegistrationDesc> registrations = GetRegistrations();
	const auto found =
	    std::ranges::find_if(registrations, [shaderName](const ShaderRegistrationDesc& desc) { return desc.ShaderName == shaderName; });
	return found != registrations.end() ? &*found : nullptr;
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindById(ShaderTypeId shaderType) noexcept
{
	const std::span<const ShaderRegistrationDesc> registrations = GetRegistrations();
	const auto found = std::ranges::lower_bound(registrations, shaderType, {}, &ShaderRegistrationDesc::TypeId);
	return found != registrations.end() && found->TypeId == shaderType ? &*found : nullptr;
}

const ShaderRegistrationDesc* GlobalShaderRegistry::FindByType(const std::type_info& shaderType) noexcept
{
	const std::span<const ShaderRegistrationDesc> registrations = GetRegistrations();
	const auto found = std::ranges::find_if(
	    registrations,
	    [&shaderType](const ShaderRegistrationDesc& candidate)
	    { return candidate.ShaderType != nullptr && *candidate.ShaderType == shaderType; });
	return found != registrations.end() ? &*found : nullptr;
}

std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor)
{
	std::string report = std::string(descriptor.Name) + " parameter(s)=" + std::to_string(descriptor.Fields.size());
	for (const ShaderParameterStructFieldDescriptor& field : descriptor.Fields)
	{
		report += "\n  name=" + std::string(field.Name);
	}
	return report;
}
