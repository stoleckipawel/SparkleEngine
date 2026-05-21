#include "PCH.h"

#include "Compiler/ShaderCompileProfile.h"

#include "RHI/Public/Config/RenderConfig.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <cstdint>
#include <string>

namespace
{
	std::uint32_t GetProfileShaderModelMinor(ShaderTarget target) noexcept
	{
		switch (target)
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

const char* ShaderCompileProfile::GetShaderModelProfileName(ShaderTarget target)
{
	switch (target)
	{
		case ShaderTarget::DxilSm60:
			return "sm_6_0";
		case ShaderTarget::DxilSm61:
			return "sm_6_1";
		case ShaderTarget::DxilSm62:
			return "sm_6_2";
		case ShaderTarget::DxilSm63:
			return "sm_6_3";
		case ShaderTarget::DxilSm64:
			return "sm_6_4";
		case ShaderTarget::DxilSm65:
			return "sm_6_5";
		case ShaderTarget::DxilSm66:
			return "sm_6_6";
		case ShaderTarget::DxilSm67:
			return "sm_6_7";
		default:
			return "sm_6_6";
	}
}

const char* ShaderCompileProfile::GetSpirVProfileName(ShaderTarget target)
{
	switch (target)
	{
		case ShaderTarget::SpirV14:
			return "spirv_1_4";
		case ShaderTarget::SpirV15:
			return "spirv_1_5";
		case ShaderTarget::SpirV16:
			return "spirv_1_6";
		default:
			return "unknown";
	}
}

const char* ShaderCompileProfile::GetSlangTargetProfileName(ShaderTarget target)
{
	return IsSpirVTarget(target) ? GetSpirVProfileName(target) : GetShaderModelProfileName(target);
}

std::string ShaderCompileProfile::BuildTargetProfile(const ShaderCompileOptions& options)
{
	std::string profile;
	profile.reserve(8);
	profile += options.PackageKind == CookedShaderPackageKind::RayTracingLibrary ? "lib" : GetShaderStagePrefix(options.Stage);
	profile += '_';
	profile += std::to_string(RenderConfig::ShaderModelMajor);
	profile += '_';
	profile += std::to_string(GetProfileShaderModelMinor(options.Target));
	return profile;
}