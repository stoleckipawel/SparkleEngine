#include "PCH.h"

#include "Compiler/ShaderCompileProfile.h"

#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <cstdint>
#include <string>

namespace
{
	std::uint32_t GetProfileShaderModelMinor(const ShaderCompileOptions& options) noexcept
	{
		switch (options.Target)
		{
			case ShaderTarget::DxilSm60:
				return 0;
			case ShaderTarget::DxilSm61:
				return 1;
			case ShaderTarget::DxilSm62:
				return 2;
			case ShaderTarget::DxilSm63:
				return 3;
			case ShaderTarget::DxilSm64:
				return 4;
			case ShaderTarget::DxilSm65:
				return 5;
			case ShaderTarget::DxilSm66:
				return 6;
			case ShaderTarget::DxilSm67:
				return 7;
			default:
				return RenderConfig::ShaderModelMinor;
		}
	}
}

std::string ShaderCompileProfile::BuildTargetProfile(const ShaderCompileOptions& options)
{
	std::string profile;
	profile.reserve(8);
	profile += options.PackageKind == CookedShaderPackageKind::RayTracingLibrary ? "lib" : GetShaderStagePrefix(options.Stage);
	profile += '_';
	profile += std::to_string(RenderConfig::ShaderModelMajor);
	profile += '_';
	profile += std::to_string(GetProfileShaderModelMinor(options));
	return profile;
}