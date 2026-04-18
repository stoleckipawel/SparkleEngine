#pragma once

#include "RHI/Public/Shaders/ShaderCompileOptions.h"
#include "RHI/Public/Shaders/ShaderCompileResult.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <ObjIdl.h>
#include <Unknwn.h>

#include <dxcapi.h>
#include <filesystem>
#include <string>

#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Standalone ShaderCompiler owns offline DXC invocation. Runtime startup must
// consume cooked shader artifacts and stay free of this compile path.
class DxcShaderCompiler
{
  public:
	static ShaderCompileResult Compile(const ShaderCompileOptions& options);

  private:
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

	static std::filesystem::path SaveShaderSymbols(IDxcResult* result, const std::filesystem::path& sourcePath);
};