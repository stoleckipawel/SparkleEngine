#include "PCH.h"

#include "Backend/ShaderBackendFactory.h"

#include "Backend/BuiltinBackends.h"

#include <algorithm>
#include <cctype>

static constexpr std::string_view kAutoShaderBackendName = "auto";

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

static std::string SelectAutomaticShaderBackendName(const std::filesystem::path& sourcePath)
{
	std::string extension = NormalizeShaderBackendName(sourcePath.extension().string());
	if (extension == ".slang")
	{
		return "slang";
	}
	if (extension == ".hlsl")
	{
		return "dxc";
	}
	return {};
}

static const ShaderBackendRegistration* FindShaderBackendRegistration(std::string_view name)
{
	const std::span<const ShaderBackendRegistration> registrations = GetBuiltinShaderBackendRegistrations();
	const auto it = std::find_if(
	    registrations.begin(),
	    registrations.end(),
	    [name](const ShaderBackendRegistration& registration) { return registration.name == name; });
	return it != registrations.end() ? &(*it) : nullptr;
}

std::vector<ShaderBackendDescriptor> ListShaderBackends()
{
	std::vector<ShaderBackendDescriptor> descriptors;
	const std::span<const ShaderBackendRegistration> registrations = GetBuiltinShaderBackendRegistrations();
	descriptors.reserve(registrations.size());
	for (const ShaderBackendRegistration& registration : registrations)
	{
		ShaderBackendDescriptor descriptor;
		descriptor.Name = registration.name;

		std::unique_ptr<IShaderBackend> backend = registration.create != nullptr ? registration.create() : nullptr;
		if (backend)
		{
			descriptor.IsAvailable = true;
			descriptor.Capabilities = backend->GetCapabilities();
			descriptor.Version = backend->GetBackendVersion();
		}

		descriptors.push_back(descriptor);
	}

	return descriptors;
}

std::unique_ptr<IShaderBackend> CreateShaderBackend(std::string_view name, std::string& outErrorMessage)
{
	const ShaderBackendRegistration* registration = FindShaderBackendRegistration(name);
	if (registration == nullptr || registration->create == nullptr)
	{
		outErrorMessage = "Unknown shader backend '" + std::string(name) + "'";
		return nullptr;
	}

	std::unique_ptr<IShaderBackend> backend = registration->create();
	if (!backend)
	{
		outErrorMessage = "Failed to construct shader backend '" + std::string(name) + "'";
		return nullptr;
	}

	outErrorMessage.clear();
	return backend;
}

std::string ResolveShaderBackendName(
	const std::filesystem::path& sourcePath,
	ShaderTarget target,
	std::string_view requestedName,
	std::string& outErrorMessage)
{
	std::string normalizedRequestedName = NormalizeShaderBackendName(requestedName);
	if (normalizedRequestedName.empty())
	{
		normalizedRequestedName = std::string(kAutoShaderBackendName);
	}

	std::string selectedName = normalizedRequestedName;
	if (selectedName == kAutoShaderBackendName)
	{
		selectedName = SelectAutomaticShaderBackendName(sourcePath);
		if (selectedName.empty())
		{
			outErrorMessage = "Unable to auto-select a shader backend for source '" + sourcePath.generic_string() + "'";
			return {};
		}
	}

	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(selectedName, outErrorMessage);
	if (!backend)
	{
		return {};
	}

	if (!backend->GetCapabilities().SupportsTarget(target))
	{
		outErrorMessage = "Shader backend '" + selectedName + "' does not support target '" + GetShaderTargetName(target) + "'";
		return {};
	}

	outErrorMessage.clear();
	return std::string(backend->GetBackendName());
}