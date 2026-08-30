#include "PCH.h"

#include "Dxc/ShaderBackendRegistration.h"

#include "Dxc/DxcShaderBackend.h"

#include <array>

#ifndef SPARKLE_SHADER_COMPILER_DXC_INCLUDE_DIR
  #define SPARKLE_SHADER_COMPILER_DXC_INCLUDE_DIR "unknown"
#endif
#ifndef SPARKLE_SHADER_COMPILER_DXC_IMPORT_LIBRARY
  #define SPARKLE_SHADER_COMPILER_DXC_IMPORT_LIBRARY "dxcompiler"
#endif
#ifndef SPARKLE_SHADER_COMPILER_DXC_RUNTIME_LIBRARY
  #define SPARKLE_SHADER_COMPILER_DXC_RUNTIME_LIBRARY "dxcompiler.dll"
#endif

static constexpr std::array<std::string_view, 1> kDxcSourceExtensions = {{".hlsl"}};
static constexpr std::array<ShaderTarget, 11> kDxcCodegenTargets = {{
    ShaderTarget::DxilSm60,
    ShaderTarget::DxilSm61,
    ShaderTarget::DxilSm62,
    ShaderTarget::DxilSm63,
    ShaderTarget::DxilSm64,
    ShaderTarget::DxilSm65,
    ShaderTarget::DxilSm66,
    ShaderTarget::DxilSm67,
    ShaderTarget::SpirV14,
    ShaderTarget::SpirV15,
    ShaderTarget::SpirV16,
}};
static constexpr std::array<std::string_view, 2> kDxcBinaryFormats = {{"Dxil", "SpirV"}};
static constexpr std::array<std::string_view, 3> kDxcDependencyLocations = {{
    "include=" SPARKLE_SHADER_COMPILER_DXC_INCLUDE_DIR,
    "importLibrary=" SPARKLE_SHADER_COMPILER_DXC_IMPORT_LIBRARY,
    "runtime=" SPARKLE_SHADER_COMPILER_DXC_RUNTIME_LIBRARY,
}};

static std::unique_ptr<IShaderBackend> CreateBackendInstance()
{
	return std::make_unique<DxcShaderBackend>();
}

static bool QueryBackendAvailability(std::string& outUnavailableReason)
{
	outUnavailableReason.clear();
	return true;
}

ShaderBackendRegistration GetDxcBackendRegistration() noexcept
{
	return ShaderBackendRegistration{
	    .Descriptor =
	        ShaderBackendStaticDescriptor{
	            .Name = "dxc",
	            .SourceExtensions = std::span<const std::string_view>(kDxcSourceExtensions.data(), kDxcSourceExtensions.size()),
	            .CodegenTargets = std::span<const ShaderTarget>(kDxcCodegenTargets.data(), kDxcCodegenTargets.size()),
	            .BinaryFormats = std::span<const std::string_view>(kDxcBinaryFormats.data(), kDxcBinaryFormats.size()),
	            .DependencyLocations = std::span<const std::string_view>(kDxcDependencyLocations.data(), kDxcDependencyLocations.size()),
	            .Capabilities = DxcShaderBackend::GetStaticCapabilities(),
	            .QueryVersion = &DxcShaderBackend::QueryBackendVersion,
	            .QueryAvailability = &QueryBackendAvailability,
	        },
	    .create = &CreateBackendInstance,
	};
}

struct DxcBackendRegistrar final
{
	DxcBackendRegistrar() { RegisterBuiltinShaderBackend(GetDxcBackendRegistration()); }
};

static DxcBackendRegistrar g_dxcBackendRegistrar;
