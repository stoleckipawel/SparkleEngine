#pragma once

#include "ShaderReflection.h"

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <ObjIdl.h>
#include <Unknwn.h>

#include <cstdint>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <span>
#include <string>

// DXIL -> ShaderReflection extractor for compiled DXC binaries.
// DXC and D3D12 reflection symbols stay confined to Backends/Dxc/.
class DxilReflectionExtractor final
{
public:
	DxilReflectionExtractor() = delete;

	static ShaderReflection Extract(IDxcUtils& utils, IDxcResult* result, std::span<const std::uint8_t> bytecode, ShaderStage stage);
	static ShaderReflection ExtractLibrary(IDxcUtils& utils, IDxcResult* result, std::string_view entryPoint);

private:
	static CookedShaderResourceKind MapResourceKind(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim);
	static CookedShaderResourceDimension MapDimension(D3D_SRV_DIMENSION dim);
	static bool IsReadOnlyKind(CookedShaderResourceKind kind);
	static CookedShaderScalarType MapScalarType(D3D_SHADER_VARIABLE_TYPE type);
	static CookedShaderScalarType MapInputComponent(D3D_REGISTER_COMPONENT_TYPE type);
	static std::uint8_t PopMaskBits(std::uint8_t mask);
};
