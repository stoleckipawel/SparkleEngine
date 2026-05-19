#include "PCH.h"

#include "DxcShaderBackend.h"

#include "Backend/ShaderBackendFactory.h"
#include "Compiler/ShaderCompileProfile.h"
#include "Compiler/ShaderCompilerPathUtils.h"
#include "Compiler/ShaderSourcePreprocessor.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "DxilReflectionExtractor.h"
#include "SpirVReflectionExtractor.h"

static const auto g_dxcShaderBackendLogger =
	Logging::GetOrCreateLogger(std::string{kDxcCompilerLoggerCategory});

DxcShaderBackend::DxcShaderBackend()
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Diagnostics::Fail(
			g_dxcShaderBackendLogger,
			__FILE__,
			__LINE__,
			"Failed to create DXC compiler instance");
		return;
	}

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Diagnostics::Fail(
			g_dxcShaderBackendLogger,
			__FILE__,
			__LINE__,
			"Failed to create DXC utils instance");
		m_compiler.Reset();
		return;
	}

	// Cache the DXC version as a packed (major, minor, commit) 64-bit value.
	// It feeds cooked backend identity and the shader artifact cache key.
	Microsoft::WRL::ComPtr<IDxcVersionInfo> versionInfo;
	if (SUCCEEDED(m_compiler.As(&versionInfo)))
	{
		UINT32 major = 0;
		UINT32 minor = 0;
		versionInfo->GetVersion(&major, &minor);
		UINT32 commit = 0;
		Microsoft::WRL::ComPtr<IDxcVersionInfo2> versionInfo2;
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
	capabilities.SupportsDxilRayTracingLibrary = IsValid();
	capabilities.SupportsDxilInlineRayQuery = IsValid();
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

	const std::filesystem::path sourcePath = ShaderCompilerPaths::CanonicalizeForCompiler(options.SourcePath);
	ShaderSourcePreprocessResult source = ShaderSourcePreprocessor::Load(sourcePath, options);
	if (!source.Succeeded())
	{
		return ShaderCompileResult::Failure(std::move(source.ErrorMessage));
	}

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = source.SourceText.data();
	sourceBuffer.Size = source.SourceText.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	std::wstring wSourcePath = ShaderCompilerPaths::MakeWidePathArgument(sourcePath);
	std::wstring wEntryPoint = Strings::ToWide(std::string_view{options.EntryPoint});
	std::wstring wTargetProfile = Strings::ToWide(std::string_view{ShaderCompileProfile::BuildTargetProfile(options)});
	std::vector<std::wstring> wIncludeDirs;
	std::vector<std::wstring> wDefines;
	std::vector<LPCWSTR> args;

	BuildCompileArguments(options, wSourcePath, wEntryPoint, wTargetProfile, wIncludeDirs, wDefines, args);

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	m_utils->CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());

	Microsoft::WRL::ComPtr<IDxcResult> result;
	HRESULT hr = m_compiler->Compile(
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

	ShaderReflection reflection;
	if (options.PackageKind != CookedShaderPackageKind::RayTracingLibrary)
	{
		std::string reflectionError;
		const bool reflectionOk = IsSpirVTarget(options.Target)
		    ? SpirVReflectionExtractor::Extract(bytecode, options.Stage, reflection, reflectionError)
		    : DxilReflectionExtractor::Extract(*m_utils.Get(), result.Get(), bytecode, options.Stage, reflection, reflectionError);
		if (!reflectionOk)
		{
			return ShaderCompileResult::Failure(
			    std::string{"DXC reflection extraction failed for target '"} + GetShaderTargetName(options.Target) +
			    "' source '" + options.SourcePath.generic_string() + "' entry '" + options.EntryPoint + "' - " + reflectionError);
		}
	}

	SPDLOG_LOGGER_INFO(
		g_dxcShaderBackendLogger,
		"Shader compiled successfully ({}): {}",
		IsSpirVTarget(options.Target) ? "SPIR-V" : "DXIL",
		options.SourcePath.filename().string());
	auto compileResult = ShaderCompileResult::Success(std::move(bytecode), debugArtifactPath);
	compileResult.SetReflection(std::move(reflection));
	if (options.CaptureDebugArtifacts)
	{
		CaptureDebugArtifacts(
			options,
			*m_utils.Get(),
			*m_compiler.Get(),
			sourceBuffer,
			bytecode,
			args,
			result.Get(),
			errorMsg,
			compileResult);
	}
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

	if (options.PackageKind != CookedShaderPackageKind::RayTracingLibrary)
	{
		outArgs.push_back(L"-E");
		outArgs.push_back(wEntryPoint.c_str());
	}

	outArgs.push_back(L"-T");
	outArgs.push_back(wTargetProfile.c_str());

	outArgs.push_back(L"-HV");
	outArgs.push_back(L"2021");

	wIncludeDirs.clear();
	wIncludeDirs.push_back(ShaderCompilerPaths::MakeWideIncludeDirectoryArgument(options.IncludeDir));
	for (const auto& dir : options.AdditionalIncludeDirs)
	{
		wIncludeDirs.push_back(ShaderCompilerPaths::MakeWideIncludeDirectoryArgument(dir));
	}
	for (const auto& dir : wIncludeDirs)
	{
		outArgs.push_back(L"-I");
		outArgs.push_back(dir.c_str());
	}

	wDefines.clear();
	for (const auto& def : options.Defines)
	{
		wDefines.push_back(Strings::ToWide(std::string_view{def}));
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
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
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
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> errorBlob;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errorBlob.ReleaseAndGetAddressOf()), nullptr);
	if (errorBlob && errorBlob->GetStringLength() > 0)
	{
		return std::string(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
	}
	return {};
}

std::string DxcShaderBackend::ExtractTextOutput(IDxcResult* result, DXC_OUT_KIND kind)
{
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> textBlob;
	if (SUCCEEDED(result->GetOutput(kind, IID_PPV_ARGS(textBlob.ReleaseAndGetAddressOf()), nullptr)) && textBlob)
	{
		return std::string(textBlob->GetStringPointer(), textBlob->GetStringLength());
	}

	Microsoft::WRL::ComPtr<IDxcBlobWide> wideBlob;
	if (SUCCEEDED(result->GetOutput(kind, IID_PPV_ARGS(wideBlob.ReleaseAndGetAddressOf()), nullptr)) && wideBlob)
	{
		return Strings::ToNarrow(std::wstring_view(wideBlob->GetStringPointer(), wideBlob->GetStringLength()));
	}

	return {};
}

std::string DxcShaderBackend::ExtractPreprocessedSource(
	IDxcUtils& utils,
	IDxcCompiler3& compiler,
	const DxcBuffer& sourceBuffer,
	const std::vector<LPCWSTR>& compileArgs)
{
	std::vector<LPCWSTR> preprocessArgs = compileArgs;
	preprocessArgs.push_back(L"-P");

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	utils.CreateDefaultIncludeHandler(includeHandler.ReleaseAndGetAddressOf());

	Microsoft::WRL::ComPtr<IDxcResult> preprocessResult;
	if (FAILED(compiler.Compile(
		    &sourceBuffer,
		    preprocessArgs.data(),
		    static_cast<UINT>(preprocessArgs.size()),
		    includeHandler.Get(),
		    IID_PPV_ARGS(preprocessResult.ReleaseAndGetAddressOf()))) || !preprocessResult)
	{
		return {};
	}

	std::string preprocessed = ExtractTextOutput(preprocessResult.Get(), DXC_OUT_HLSL);
	if (!preprocessed.empty())
	{
		return preprocessed;
	}

	return {};
}

std::string DxcShaderBackend::ExtractDisassembly(
	IDxcUtils& utils,
	IDxcCompiler3& compiler,
	std::span<const std::uint8_t> bytecode)
{
	if (bytecode.empty())
	{
		return {};
	}

	DxcBuffer objectBuffer{};
	objectBuffer.Ptr = bytecode.data();
	objectBuffer.Size = bytecode.size();
	objectBuffer.Encoding = 0;

	Microsoft::WRL::ComPtr<IDxcResult> disassemblyResult;
	if (FAILED(compiler.Disassemble(&objectBuffer, IID_PPV_ARGS(disassemblyResult.ReleaseAndGetAddressOf()))) || !disassemblyResult)
	{
		return {};
	}

	std::string disassembly = ExtractTextOutput(disassemblyResult.Get(), DXC_OUT_DISASSEMBLY);
	if (!disassembly.empty())
	{
		return disassembly;
	}

	Microsoft::WRL::ComPtr<IDxcCompiler> disassemblyCompiler;
	if (FAILED(compiler.QueryInterface(IID_PPV_ARGS(disassemblyCompiler.ReleaseAndGetAddressOf()))) || !disassemblyCompiler)
	{
		return {};
	}

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> objectBlob;
	if (FAILED(utils.CreateBlobFromPinned(
		    bytecode.data(),
		    static_cast<UINT32>(bytecode.size()),
		    0,
		    objectBlob.ReleaseAndGetAddressOf())) || !objectBlob)
	{
		return {};
	}

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> disassemblyBlob;
	if (FAILED(disassemblyCompiler->Disassemble(objectBlob.Get(), disassemblyBlob.ReleaseAndGetAddressOf())) || !disassemblyBlob)
	{
		return {};
	}

	return std::string(
	    static_cast<const char*>(disassemblyBlob->GetBufferPointer()),
	    static_cast<std::size_t>(disassemblyBlob->GetBufferSize()));
}

void DxcShaderBackend::CaptureDebugArtifacts(
	const ShaderCompileOptions& options,
	IDxcUtils& utils,
	IDxcCompiler3& compiler,
	const DxcBuffer& sourceBuffer,
	std::span<const std::uint8_t> bytecode,
	const std::vector<LPCWSTR>& compileArgs,
	IDxcResult* result,
	std::string_view compilerOutput,
	ShaderCompileResult& outCompileResult)
{
	ShaderDebugArtifactSet debugArtifacts;
	debugArtifacts.CompileArguments = BuildDebugArgumentStrings(compileArgs);
	debugArtifacts.CompilerOutput.assign(compilerOutput);
	debugArtifacts.Disassembly = ExtractDisassembly(utils, compiler, bytecode);
	debugArtifacts.PreprocessedSource = ExtractPreprocessedSource(utils, compiler, sourceBuffer, compileArgs);
	if (debugArtifacts.PreprocessedSource.empty())
	{
		std::vector<std::uint8_t> sourceBytes;
		std::string sourceError;
		if (Files::TryReadAllBytes(options.SourcePath, sourceBytes, sourceError))
		{
			debugArtifacts.PreprocessedSource.assign(reinterpret_cast<const char*>(sourceBytes.data()), sourceBytes.size());
		}
	}
	outCompileResult.SetDebugArtifacts(std::move(debugArtifacts));
}

std::vector<std::string> DxcShaderBackend::BuildDebugArgumentStrings(const std::vector<LPCWSTR>& compileArgs)
{
	std::vector<std::string> args;
	args.reserve(compileArgs.size());
	for (const LPCWSTR arg : compileArgs)
	{
		args.push_back(Strings::ToNarrow(std::wstring_view(arg)));
	}
	return args;
}

std::filesystem::path DxcShaderBackend::SaveShaderSymbols(IDxcResult* result, const std::filesystem::path& sourcePath)
{
	Microsoft::WRL::ComPtr<IDxcBlob> pdbBlob;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> pdbNameBlob;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdbBlob.ReleaseAndGetAddressOf()), pdbNameBlob.ReleaseAndGetAddressOf());

	if (!pdbBlob || !pdbNameBlob)
		return {};

	std::wstring pdbName(pdbNameBlob->GetStringPointer());
	const std::filesystem::path pdbPath = Paths::ShaderSymbolsOutputRoot() / std::filesystem::path(pdbName).filename();

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
