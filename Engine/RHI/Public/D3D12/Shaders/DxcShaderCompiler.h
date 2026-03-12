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
	static ShaderCompileResult Compile(const AssetSystem& assetSystem, const ShaderCompileOptions& options);

	static ShaderCompileResult CompileFromAsset(
	    const AssetSystem& assetSystem,
	    const std::filesystem::path& sourcePath,
	    ShaderStage stage,
	    const std::string& entryPoint = "main");

  private:
	static void ConfigureIncludePaths(const AssetSystem& assetSystem, ShaderCompileOptions& options);

	static void ApplyBuildConfiguration(ShaderCompileOptions& options);

	static void BuildCompileArguments(
	    const ShaderCompileOptions& options,
	    const std::wstring& wSourcePath,
	    const std::wstring& wEntryPoint,
	    const std::wstring& wTargetProfile,
	    std::vector<std::wstring>& wIncludeDirs,
	    std::vector<std::wstring>& wDefines,
	    std::vector<LPCWSTR>& outArgs);

	static std::vector<uint8_t> ExtractBytecode(IDxcResult* result);

	static std::string ExtractErrorMessage(IDxcResult* result);

	static void SaveShaderSymbols(const AssetSystem& assetSystem, IDxcResult* result, const std::filesystem::path& sourcePath);
};
