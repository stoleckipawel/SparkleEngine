#pragma once

#include "Backend/IShaderBackend.h"

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
#include <span>
#include <string>
#include <vector>
#include <wrl/client.h>

// DXC backend used privately by ShaderPackageCooker.
// Produces both DXIL and SPIR-V via DXC's `-spirv` mode.
class DxcShaderBackend final : public IShaderBackend
{
  public:
	DxcShaderBackend();
	~DxcShaderBackend() override = default;

	DxcShaderBackend(const DxcShaderBackend&) = delete;
	DxcShaderBackend& operator=(const DxcShaderBackend&) = delete;
	DxcShaderBackend(DxcShaderBackend&&) = delete;
	DxcShaderBackend& operator=(DxcShaderBackend&&) = delete;

	bool IsValid() const noexcept { return m_compiler != nullptr && m_utils != nullptr; }

	ShaderBackendCapabilities GetCapabilities() const override;

	std::string_view GetBackendName() const override;
	std::uint64_t GetBackendVersion() const override;

	ShaderCompileResult Compile(const ShaderCompileOptions& options) override;

  private:
	static void BuildCompileArguments(
	    const ShaderCompileOptions& options,
	    const std::wstring& wSourcePath,
	    const std::wstring& wEntryPoint,
	    const std::wstring& wTargetProfile,
	    std::vector<std::wstring>& wIncludeDirs,
	    std::vector<std::wstring>& wDefines,
	    std::vector<LPCWSTR>& outArgs);

	static std::vector<std::uint8_t> ExtractBytecode(IDxcResult* result);

	static std::string ExtractErrorMessage(IDxcResult* result);
	static std::string ExtractTextOutput(IDxcResult* result, DXC_OUT_KIND kind);
	static std::string ExtractPreprocessedSource(
	    IDxcUtils& utils,
	    IDxcCompiler3& compiler,
	    const DxcBuffer& sourceBuffer,
	    const std::vector<LPCWSTR>& compileArgs);
	static std::string ExtractDisassembly(
	    IDxcUtils& utils,
	    IDxcCompiler3& compiler,
	    std::span<const std::uint8_t> bytecode);
	static void CaptureDebugArtifacts(
	    const ShaderCompileOptions& options,
	    IDxcUtils& utils,
	    IDxcCompiler3& compiler,
	    const DxcBuffer& sourceBuffer,
	    std::span<const std::uint8_t> bytecode,
	    const std::vector<LPCWSTR>& compileArgs,
	    IDxcResult* result,
	    std::string_view compilerOutput,
	    ShaderCompileResult& outCompileResult);
	static std::vector<std::string> BuildDebugArgumentStrings(const std::vector<LPCWSTR>& compileArgs);

	static std::filesystem::path SaveShaderSymbols(IDxcResult* result, const std::filesystem::path& sourcePath);

	static std::filesystem::path BuildShaderDebugArtifactPath(std::wstring_view pdbName);

	Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
	Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
	std::uint64_t m_backendVersion = 0;
};
