#pragma once

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendCapabilities.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct ShaderBackendDescriptor final
{
	std::string_view Name;
	ShaderBackendCapabilities Capabilities;
	std::uint64_t Version = 0;
	bool IsAvailable = false;
};

std::vector<ShaderBackendDescriptor> ListShaderBackends();
std::unique_ptr<IShaderBackend> CreateShaderBackend(std::string_view name, std::string& outErrorMessage);
std::string ResolveShaderBackendName(
	const std::filesystem::path& sourcePath,
	ShaderTarget target,
	std::string_view requestedName,
	std::string& outErrorMessage);
