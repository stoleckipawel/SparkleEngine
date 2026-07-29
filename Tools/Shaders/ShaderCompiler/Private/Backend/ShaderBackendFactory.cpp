#include "PCH.h"

#include "Backend/ShaderBackendFactory.h"

#include "Backend/BuiltinBackends.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>
#include <array>
#include <cctype>

static constexpr std::string_view kAutoShaderBackendName = "auto";
static constexpr std::array<ShaderBinaryFormatDescriptor, 2> kShaderBinaryFormats = {{
	{.Name = "Dxil", .IsAvailable = true},
	{.Name = "SpirV", .IsAvailable = true},
}};
static constexpr std::array<ShaderCodegenTargetDescriptor, 11> kShaderCodegenTargets = {{
	{.Target = ShaderTarget::DxilSm60, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm61, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm62, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm63, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm64, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm65, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm66, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::DxilSm67, .BinaryFormat = "Dxil", .IsAvailable = true},
	{.Target = ShaderTarget::SpirV14, .BinaryFormat = "SpirV", .IsAvailable = true},
	{.Target = ShaderTarget::SpirV15, .BinaryFormat = "SpirV", .IsAvailable = true},
	{.Target = ShaderTarget::SpirV16, .BinaryFormat = "SpirV", .IsAvailable = true},
}};

static std::string NormalizeShaderBackendName(std::string_view name)
{
	std::string normalized(name);
	std::transform(
	    normalized.begin(),
	    normalized.end(),
	    normalized.begin(),
	    [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
	return normalized;
}

static bool HasSourceExtension(const ShaderBackendStaticDescriptor& descriptor, std::string_view extension) noexcept
{
	return std::find(descriptor.SourceExtensions.begin(), descriptor.SourceExtensions.end(), extension) !=
	       descriptor.SourceExtensions.end();
}

static std::string SelectAutomaticShaderBackendName(const std::filesystem::path& sourcePath, ShaderTarget target)
{
	const std::string extension = NormalizeShaderBackendName(sourcePath.extension().string());
	const std::span<const ShaderBackendRegistration> registrations = GetBuiltinShaderBackendRegistrations();
	for (const ShaderBackendRegistration& registration : registrations)
	{
		if (HasSourceExtension(registration.Descriptor, extension) && registration.Descriptor.Capabilities.SupportsTarget(target))
		{
			return std::string(registration.Descriptor.Name);
		}
	}
	return {};
}

static const ShaderBackendRegistration* FindShaderBackendRegistration(std::string_view name)
{
	const std::span<const ShaderBackendRegistration> registrations = GetBuiltinShaderBackendRegistrations();
	const auto it = std::find_if(
	    registrations.begin(),
	    registrations.end(),
	    [name](const ShaderBackendRegistration& registration) { return registration.Descriptor.Name == name; });
	return it != registrations.end() ? &(*it) : nullptr;
}

static bool QueryBackendAvailability(const ShaderBackendStaticDescriptor& descriptor, std::string& outUnavailableReason)
{
	if (descriptor.QueryAvailability == nullptr)
	{
		outUnavailableReason.clear();
		return true;
	}
	return descriptor.QueryAvailability(outUnavailableReason);
}

std::vector<ShaderBackendDescriptor> ListShaderBackends()
{
	std::vector<ShaderBackendDescriptor> descriptors;
	const std::span<const ShaderBackendRegistration> registrations = GetBuiltinShaderBackendRegistrations();
	descriptors.reserve(registrations.size());
	for (const ShaderBackendRegistration& registration : registrations)
	{
		ShaderBackendDescriptor descriptor;
		descriptor.Name = registration.Descriptor.Name;
		descriptor.SourceExtensions = registration.Descriptor.SourceExtensions;
		descriptor.CodegenTargets = registration.Descriptor.CodegenTargets;
		descriptor.BinaryFormats = registration.Descriptor.BinaryFormats;
		descriptor.DependencyLocations = registration.Descriptor.DependencyLocations;
		descriptor.Capabilities = registration.Descriptor.Capabilities;
		descriptor.IsAvailable = QueryBackendAvailability(registration.Descriptor, descriptor.UnavailableReason);
		descriptor.Version = registration.Descriptor.QueryVersion != nullptr ? registration.Descriptor.QueryVersion() : 0;

		descriptors.push_back(descriptor);
	}

	return descriptors;
}

std::span<const ShaderBinaryFormatDescriptor> ListShaderBinaryFormats() noexcept
{
	return kShaderBinaryFormats;
}

std::span<const ShaderCodegenTargetDescriptor> ListShaderCodegenTargets() noexcept
{
	return kShaderCodegenTargets;
}

std::unique_ptr<IShaderBackend> CreateShaderBackend(std::string_view name)
{
	const ShaderBackendRegistration* registration = FindShaderBackendRegistration(NormalizeShaderBackendName(name));
	if (registration == nullptr || registration->create == nullptr)
	{
		throw Diagnostics::Error("Unknown shader backend '" + std::string(name) + "'.");
	}

	std::unique_ptr<IShaderBackend> backend = registration->create();
	if (!backend)
	{
		throw Diagnostics::Error("Failed to construct shader backend '" + std::string(registration->Descriptor.Name) + "'.");
	}

	return backend;
}

std::string ResolveShaderBackendName(
	const std::filesystem::path& sourcePath,
	ShaderTarget target,
	std::string_view requestedName)
{
	std::string normalizedRequestedName = NormalizeShaderBackendName(requestedName);
	if (normalizedRequestedName.empty())
	{
		normalizedRequestedName = std::string(kAutoShaderBackendName);
	}

	std::string selectedName = normalizedRequestedName;
	if (selectedName == kAutoShaderBackendName)
	{
		selectedName = SelectAutomaticShaderBackendName(sourcePath, target);
		if (selectedName.empty())
		{
			throw Diagnostics::Error(
			    "Unable to auto-select a shader backend for source '" + sourcePath.generic_string() +
			    "' and target '" + GetShaderTargetName(target) + "'.");
		}
	}

	const ShaderBackendRegistration* registration = FindShaderBackendRegistration(selectedName);
	if (registration == nullptr)
	{
		throw Diagnostics::Error("Unknown shader backend '" + selectedName + "'.");
	}

	std::string unavailableReason;
	if (!QueryBackendAvailability(registration->Descriptor, unavailableReason))
	{
		std::string diagnostic = "Shader backend '" + selectedName + "' is unavailable";
		if (!unavailableReason.empty())
		{
			diagnostic += ": " + unavailableReason;
		}
		throw Diagnostics::Error(std::move(diagnostic));
	}

	if (!registration->Descriptor.Capabilities.SupportsTarget(target))
	{
		throw Diagnostics::Error(
		    "Shader backend '" + selectedName + "' does not support target '" + GetShaderTargetName(target) + "'.");
	}

	return std::string(registration->Descriptor.Name);
}
