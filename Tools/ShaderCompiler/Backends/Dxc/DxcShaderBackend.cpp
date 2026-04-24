#include "PCH.h"

#include "DxcShaderBackend.h"

#include "Backend/ShaderBackendFactory.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "DxilReflectionExtractor.h"
#include "SpirVReflectionExtractor.h"

using Microsoft::WRL::ComPtr;

static const auto g_dxcShaderBackendLogger =
	Engine::Logging::GetOrCreateLogger(std::string{kDxcCompilerLoggerCategory});

DxcShaderBackend::DxcShaderBackend()
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Engine::Diagnostics::Fail(
			g_dxcShaderBackendLogger,
			__FILE__,
			__LINE__,
			"Failed to create DXC compiler instance");
		return;
	}

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Engine::Diagnostics::Fail(
			g_dxcShaderBackendLogger,
			__FILE__,
			__LINE__,
			"Failed to create DXC utils instance");
		m_compiler.Reset();
		return;
	}

	// Cache the DXC version as a packed (major, minor, commit) 64-bit value.
	// It feeds cooked backend identity and the shader artifact cache key.
	ComPtr<IDxcVersionInfo> versionInfo;
	if (SUCCEEDED(m_compiler.As(&versionInfo)))
	{
		UINT32 major = 0;
		UINT32 minor = 0;
		versionInfo->GetVersion(&major, &minor);
		UINT32 commit = 0;
		ComPtr<IDxcVersionInfo2> versionInfo2;
		if (SUCCEEDED(versionInfo.As(&versionInfo2)))
		{
			char* commitHash = nullptr;
			versionInfo2->GetCommitInfo(&commit, &commitHash);
			if (commitHash != nullptr)
			{
				CoTaskMemFree(commitHash);
			}
		}
		m_backendVersion = (static_cast<std::uint64_t>(major) << 48u) | (static_cast<std::uint64_t>(minor) << 32u) |
		                   static_cast<std::uint64_t>(commit);
	}
}

ShaderBackendCapabilities DxcShaderBackend::GetCapabilities() const
{
	ShaderBackendCapabilities capabilities;
	capabilities.SupportsDxil = IsValid();
	capabilities.SupportsSpirV = IsValid();
	return capabilities;
}

std::string_view DxcShaderBackend::GetBackendName() const
{
	return "dxc";
}

std::uint64_t DxcShaderBackend::GetBackendVersion() const
{
	return m_backendVersion;
}

ShaderCompileResult DxcShaderBackend::Compile(const ShaderCompileOptions& options)
{
	if (!IsValid())
	{
		return ShaderCompileResult::Failure("DXC backend is not initialized");
	}

	ComPtr<IDxcBlobEncoding> sourceBlob;
	HRESULT hr = m_utils->LoadFile(options.SourcePath.c_str(), nullptr, sourceBlob.ReleaseAndGetAddressOf());
	if (FAILED(hr) || !sourceBlob)
	{
		return ShaderCompileResult::Failure("Failed to load shader source: " + options.SourcePath.string());
	}

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP;

	std::wstring wSourcePath = Engine::Strings::ToWide(options.SourcePath);
	std::wstring wEntryPoint = Engine::Strings::ToWide(std::string_view{options.EntryPoint});
	std::wstring wTargetProfile = Engine::Strings::ToWide(std::string_view{options.BuildTargetProfile()});
	std::vector<std::wstring> wIncludeDirs;
	std::vector<std::wstring> wDefines;
	std::vector<LPCWSTR> args;

	BuildCompileArguments(options, wSourcePath, wEntryPoint, wTargetProfile, wIncludeDirs, wDefines, args);

	ComPtr<IDxcIncludeHandler> includeHandler;
	m_utils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());

	ComPtr<IDxcResult> result;
	hr = m_compiler->Compile(
	    &sourceBuffer,
	    args.data(),
	    static_cast<UINT>(args.size()),
	    includeHandler.Get(),
	    IID_PPV_ARGS(result.ReleaseAndGetAddressOf()));

	if (FAILED(hr) || !result)
	{
		return ShaderCompileResult::Failure("DXC Compile() call failed");
	}

	std::string errorMsg = ExtractErrorMessage(result.Get());

	HRESULT status;
	result->GetStatus(&status);
	if (FAILED(status))
	{
		if (errorMsg.empty())
			errorMsg = "Compilation failed with no error message";
		SPDLOG_LOGGER_ERROR(g_dxcShaderBackendLogger, "Shader compilation failed: {}", errorMsg);
		return ShaderCompileResult::Failure(std::move(errorMsg));
	}

	if (!errorMsg.empty())
	{
		SPDLOG_LOGGER_WARN(g_dxcShaderBackendLogger, "Shader warnings: {}", errorMsg);
	}

	std::vector<std::uint8_t> bytecode = ExtractBytecode(result.Get());
	if (bytecode.empty())
	{
		return ShaderCompileResult::Failure("Failed to extract shader bytecode");
	}

	// PDBs only meaningful for DXIL today; SPIR-V output does not produce a
	// DXC PDB blob, so SaveShaderSymbols returns an empty path harmlessly.
	const std::filesystem::path debugArtifactPath = SaveShaderSymbols(result.Get(), options.SourcePath);

	// Reflection failures stay non-fatal.
	// The binary is still valid; the stage just lands with empty reflection.
	ShaderReflection reflection;
	std::string reflectionError;
	const bool reflectionOk = IsSpirVTarget(options.Target)
	    ? SpirVReflectionExtractor::Extract(bytecode, options.Stage, reflection, reflectionError)
	    : DxilReflectionExtractor::Extract(*m_utils.Get(), result.Get(), bytecode, options.Stage, reflection, reflectionError);
	if (!reflectionOk)
	{
		SPDLOG_LOGGER_WARN(
			g_dxcShaderBackendLogger,
			"Shader reflection extraction failed for '{}': {}",
			options.SourcePath.filename().string(),
			reflectionError);
	}

	SPDLOG_LOGGER_INFO(
		g_dxcShaderBackendLogger,
		"Shader compiled successfully ({}): {}",
		IsSpirVTarget(options.Target) ? "SPIR-V" : "DXIL",
		options.SourcePath.filename().string());
	auto compileResult = ShaderCompileResult::Success(std::move(bytecode), debugArtifactPath);
	compileResult.SetReflection(std::move(reflection));
	return compileResult;
}

void DxcShaderBackend::BuildCompileArguments(
	const ShaderCompileOptions& options,
	const std::wstring& wSourcePath,
	const std::wstring& wEntryPoint,
	const std::wstring& wTargetProfile,
	std::vector<std::wstring>& wIncludeDirs,
	std::vector<std::wstring>& wDefines,
	std::vector<LPCWSTR>& outArgs)
{
	outArgs.clear();
	outArgs.reserve(32);

	outArgs.push_back(wSourcePath.c_str());

	outArgs.push_back(L"-E");
	outArgs.push_back(wEntryPoint.c_str());

	outArgs.push_back(L"-T");
	outArgs.push_back(wTargetProfile.c_str());

	outArgs.push_back(L"-HV");
	outArgs.push_back(L"2021");

	wIncludeDirs.clear();
	wIncludeDirs.push_back(Engine::Strings::ToWide(options.IncludeDir));
	for (const auto& dir : options.AdditionalIncludeDirs)
	{
		wIncludeDirs.push_back(Engine::Strings::ToWide(dir));
	}
	for (const auto& dir : wIncludeDirs)
	{
		outArgs.push_back(L"-I");
		outArgs.push_back(dir.c_str());
	}

	wDefines.clear();
	for (const auto& def : options.Defines)
	{
		wDefines.push_back(Engine::Strings::ToWide(std::string_view{def}));
	}
	for (const auto& def : wDefines)
	{
		outArgs.push_back(L"-D");
		outArgs.push_back(def.c_str());
	}

	outArgs.push_back(DXC_ARG_ENABLE_STRICTNESS);
	outArgs.push_back(DXC_ARG_ALL_RESOURCES_BOUND);

	if (options.TreatWarningsAsErrors)
	{
		outArgs.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
	}

	// -Qstrip_reflect / -Qstrip_debug are DXIL-only DXC flags; SPIR-V mode
	// rejects them, so only emit them when targeting DXIL.
	if (!IsSpirVTarget(options.Target))
	{
		if (options.StripReflection)
		{
			outArgs.push_back(L"-Qstrip_reflect");
		}
		if (options.StripDebugInfo)
		{
			outArgs.push_back(L"-Qstrip_debug");
		}
	}

	if (options.EnableDebugInfo)
	{
		outArgs.push_back(DXC_ARG_DEBUG);
	}

	if (options.EnableOptimizations)
	{
		outArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
	}
	else
	{
		outArgs.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
	}

	// SPIR-V mode toggle. DXC emits SPIR-V when -spirv is present; the same
	// HLSL profile string still selects the shader stage.
	if (IsSpirVTarget(options.Target))
	{
		outArgs.push_back(L"-spirv");
	}
}

std::vector<std::uint8_t> DxcShaderBackend::ExtractBytecode(IDxcResult* result)
{
	ComPtr<IDxcBlob> shaderBlob;
	HRESULT hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.ReleaseAndGetAddressOf()), nullptr);
	if (FAILED(hr) || !shaderBlob || shaderBlob->GetBufferSize() == 0)
	{
		return {};
	}

	const std::uint8_t* data = static_cast<const std::uint8_t*>(shaderBlob->GetBufferPointer());
	return std::vector<std::uint8_t>(data, data + shaderBlob->GetBufferSize());
}

std::string DxcShaderBackend::ExtractErrorMessage(IDxcResult* result)
{
	ComPtr<IDxcBlobUtf8> errorBlob;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errorBlob.ReleaseAndGetAddressOf()), nullptr);
	if (errorBlob && errorBlob->GetStringLength() > 0)
	{
		return std::string(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
	}
	return {};
}

std::filesystem::path DxcShaderBackend::SaveShaderSymbols(IDxcResult* result, const std::filesystem::path& sourcePath)
{
	ComPtr<IDxcBlob> pdbBlob;
	ComPtr<IDxcBlobUtf16> pdbNameBlob;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdbBlob.ReleaseAndGetAddressOf()), pdbNameBlob.ReleaseAndGetAddressOf());

	if (!pdbBlob || !pdbNameBlob)
		return {};

	std::wstring pdbName(pdbNameBlob->GetStringPointer());
	const std::filesystem::path pdbPath = BuildShaderDebugArtifactPath(pdbName);

	FILE* fp = nullptr;
	_wfopen_s(&fp, pdbPath.c_str(), L"wb");
	if (fp)
	{
		fwrite(pdbBlob->GetBufferPointer(), 1, pdbBlob->GetBufferSize(), fp);
		fclose(fp);
		return pdbPath;
	}

	SPDLOG_LOGGER_WARN(g_dxcShaderBackendLogger, "Failed to save shader symbols for '{}'", sourcePath.string());
	return {};
}

std::filesystem::path DxcShaderBackend::BuildShaderDebugArtifactPath(std::wstring_view pdbName)
{
	return Filesystem::GetShaderSymbolsOutputPath() / std::filesystem::path(pdbName).filename();
}

std::unique_ptr<IShaderBackend> CreateDefaultShaderBackend()
{
	return std::make_unique<DxcShaderBackend>();
}
