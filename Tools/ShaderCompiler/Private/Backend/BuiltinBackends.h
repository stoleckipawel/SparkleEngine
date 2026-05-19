#pragma once

#include "Backend/IShaderBackend.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>

struct ShaderBackendStaticDescriptor final
{
	std::string_view Name;
	bool IsRequired = true;
	std::span<const std::string_view> SourceExtensions;
	std::span<const ShaderTarget> CodegenTargets;
	std::span<const std::string_view> BinaryFormats;
	std::span<const std::string_view> DependencyLocations;
	ShaderBackendCapabilities Capabilities;
	std::uint64_t (*QueryVersion)() = nullptr;
	bool (*QueryAvailability)(std::string& outUnavailableReason) = nullptr;
};

struct ShaderBackendRegistration final
{
	ShaderBackendStaticDescriptor Descriptor;
	std::unique_ptr<IShaderBackend> (*create)();
};

void RegisterBuiltinShaderBackend(ShaderBackendRegistration registration);
std::span<const ShaderBackendRegistration> GetBuiltinShaderBackendRegistrations() noexcept;