#pragma once

#include <cstdint>

namespace CookedShaderPackageContract
{
	inline constexpr std::uint16_t ShaderModelMajor = 6u;
	inline constexpr std::uint16_t ShaderModelMinor = 6u;
	// Packages may contain additional offline targets; runtime backends select
	// only these canonical artifacts.
	inline constexpr char DxilRuntimeCodegenTarget[] = "DxilSm66";
	inline constexpr char SpirVRuntimeCodegenTarget[] = "SpirV16";
}
