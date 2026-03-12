// ============================================================================
// DxcShaderCompiler.h
// ----------------------------------------------------------------------------
// Compiles HLSL shaders using the DirectX Shader Compiler (DXC).
//
#pragma once

#include "ShaderCompileOptions.h"
#include "ShaderCompileResult.h"
#include <dxcapi.h>
#include <wrl/client.h>
#include <vector>
#include <string>

using Microsoft::WRL::ComPtr;

class AssetSystem;

class DxcShaderCompiler
{
  public:
	// Compiles a shader with the given options.
	// Returns a result containing bytecode on success, or error message on failure.
	static ShaderCompileResult Compile(const AssetSystem& assetSystem, const ShaderCompileOptions& options);

	// Convenience overload: resolves the shader path and builds options automatically.
	// sourcePath: relative path from shader root (e.g., "Passes/Forward/ForwardLitVS.hlsl")
	static ShaderCompileResult CompileFromAsset(
	    const AssetSystem& assetSystem,
	    const std::filesystem::path& sourcePath,
	    ShaderStage stage,
	    const std::string& entryPoint = "main");

  private:
	// Configures include directories for shader compilation with project-override-engine semantics.
	static void ConfigureIncludePaths(const AssetSystem& assetSystem, ShaderCompileOptions& options);

	// Applies build configuration (debug/optimization flags) based on engine defines.
	static void ApplyBuildConfiguration(ShaderCompileOptions& options);

	// Builds the DXC argument list from compile options.
	// Arguments reference strings in the storage vectors - those must outlive the args vector.
	static void BuildCompileArguments(
	    const ShaderCompileOptions& options,
	    const std::wstring& wSourcePath,
	    const std::wstring& wEntryPoint,
	    const std::wstring& wTargetProfile,
	    std::vector<std::wstring>& wIncludeDirs,
	    std::vector<std::wstring>& wDefines,
	    std::vector<LPCWSTR>& outArgs);

	// Extracts bytecode from a successful compilation result.
	static std::vector<uint8_t> ExtractBytecode(IDxcResult* result);

	// Extracts error/warning messages from compilation output.
	static std::string ExtractErrorMessage(IDxcResult* result);

	// Saves shader symbols (PDB) to disk for debugging.
	static void SaveShaderSymbols(const AssetSystem& assetSystem, IDxcResult* result, const std::filesystem::path& sourcePath);
};
