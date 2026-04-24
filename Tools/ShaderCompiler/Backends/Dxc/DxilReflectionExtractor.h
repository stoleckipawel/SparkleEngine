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

	// Populates `outReflection` and returns false with `outError` on failure.
	// Extraction errors are logged by the caller and do not fail compilation.
	static bool Extract(
	    IDxcUtils& utils,
	    IDxcResult* result,
	    std::span<const std::uint8_t> bytecode,
	    ShaderStage stage,
	    ShaderReflection& outReflection,
	    std::string& outError);

  private:
	static CookedShaderResourceKind MapResourceKind(D3D_SHADER_INPUT_TYPE type, D3D_SRV_DIMENSION dim);
	static CookedShaderResourceDimension MapDimension(D3D_SRV_DIMENSION dim);
	static bool IsReadOnlyKind(CookedShaderResourceKind kind);
	static CookedShaderScalarType MapScalarType(D3D_SHADER_VARIABLE_TYPE type);
	static CookedShaderScalarType MapInputComponent(D3D_REGISTER_COMPONENT_TYPE type);
	static std::uint8_t PopMaskBits(std::uint8_t mask);
};
