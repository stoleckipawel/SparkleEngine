#pragma once

#include <cstdint>

// Backend target families.
// Today all Dxil* and SpirV* values use the global shader-model setting.
enum class ShaderTarget : std::uint16_t
{
	DxilSm60 = 0,
	DxilSm61,
	DxilSm62,
	DxilSm63,
	DxilSm64,
	DxilSm65,
	DxilSm66,
	DxilSm67,
	SpirV14,
	SpirV15,
	SpirV16,
};

constexpr ShaderTarget kDefaultShaderTarget = ShaderTarget::DxilSm66;

constexpr bool IsDxilTarget(ShaderTarget target) noexcept
{
	return target >= ShaderTarget::DxilSm60 && target <= ShaderTarget::DxilSm67;
}

constexpr bool IsSpirVTarget(ShaderTarget target) noexcept
{
	return target >= ShaderTarget::SpirV14 && target <= ShaderTarget::SpirV16;
}

constexpr const char* GetShaderTargetName(ShaderTarget target) noexcept
{
	switch (target)
	{
		case ShaderTarget::DxilSm60: return "DxilSm60";
		case ShaderTarget::DxilSm61: return "DxilSm61";
		case ShaderTarget::DxilSm62: return "DxilSm62";
		case ShaderTarget::DxilSm63: return "DxilSm63";
		case ShaderTarget::DxilSm64: return "DxilSm64";
		case ShaderTarget::DxilSm65: return "DxilSm65";
		case ShaderTarget::DxilSm66: return "DxilSm66";
		case ShaderTarget::DxilSm67: return "DxilSm67";
		case ShaderTarget::SpirV14:  return "SpirV14";
		case ShaderTarget::SpirV15:  return "SpirV15";
		case ShaderTarget::SpirV16:  return "SpirV16";
	}
	return "Unknown";
}