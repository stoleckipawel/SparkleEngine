#pragma once

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendCapabilities.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct ShaderBackendDescriptor final
{
	std::string_view Name;
	std::span<const std::string_view> SourceExtensions;
	std::span<const ShaderTarget> CodegenTargets;
	std::span<const std::string_view> BinaryFormats;
	std::span<const std::string_view> DependencyLocations;
	ShaderBackendCapabilities Capabilities;
	std::uint64_t Version = 0;
	bool IsAvailable = false;
	std::string UnavailableReason;
};

struct ShaderBinaryFormatDescriptor final
{
	std::string_view Name;
	bool IsAvailable = true;
};

struct ShaderCodegenTargetDescriptor final
{
	ShaderTarget Target = kDefaultShaderTarget;
	std::string_view BinaryFormat;
	bool IsAvailable = true;
};

std::vector<ShaderBackendDescriptor> ListShaderBackends();
std::span<const ShaderBinaryFormatDescriptor> ListShaderBinaryFormats() noexcept;
std::span<const ShaderCodegenTargetDescriptor> ListShaderCodegenTargets() noexcept;
std::unique_ptr<IShaderBackend> CreateShaderBackend(std::string_view name);
std::string ResolveShaderBackendName(std::string_view sourcePath, ShaderTarget target, std::string_view requestedName);
