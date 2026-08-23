#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "DxcShaderBackend.h"

#include "SpirVBindingNormalizer.h"

#include "Backend/ShaderBackendFactory.h"
#include "Compiler/ShaderCompileProfile.h"
#include "Compiler/ShaderSourceMountTable.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "DxilReflectionExtractor.h"
#include "SpirVReflectionExtractor.h"

static const auto g_dxcShaderBackendLogger = Logging::GetOrCreateLogger(std::string{kDxcCompilerLoggerCategory});

DxcShaderBackend::DxcShaderBackend()
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Diagnostics::Fatal(g_dxcShaderBackendLogger, __FILE__, __LINE__, "Failed to create DXC compiler instance");
		return;
	}

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		Diagnostics::Fatal(g_dxcShaderBackendLogger, __FILE__, __LINE__, "Failed to create DXC utils instance");
		m_compiler.Reset();
		return;
	}

	m_backendVersion = QueryBackendVersion(*m_compiler.Get());
}

ShaderBackendCapabilities DxcShaderBackend::GetStaticCapabilities() noexcept
{
	ShaderBackendCapabilities capabilities;
	capabilities.SupportsDxil = true;
	capabilities.SupportsSpirV = true;
	capabilities.SupportsDxilRayTracingLibrary = true;
	capabilities.SupportsDxilInlineRayQuery = true;
	capabilities.SupportsSpirVInlineRayQuery = true;
	return capabilities;
}

std::uint64_t DxcShaderBackend::QueryBackendVersion()
{
	Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf()))) || !compiler)
	{
		return 0;
	}
	return QueryBackendVersion(*compiler.Get());
}

std::uint64_t DxcShaderBackend::QueryBackendVersion(IDxcCompiler3& compiler)
{
	Microsoft::WRL::ComPtr<IDxcVersionInfo> versionInfo;
	if (FAILED(compiler.QueryInterface(IID_PPV_ARGS(versionInfo.ReleaseAndGetAddressOf()))) || !versionInfo)
	{
		return 0;
	}

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
	return (static_cast<std::uint64_t>(major) << 48u) | (static_cast<std::uint64_t>(minor) << 32u) | static_cast<std::uint64_t>(commit);
}

ShaderBackendCapabilities DxcShaderBackend::GetCapabilities() const
{
	ShaderBackendCapabilities capabilities = GetStaticCapabilities();
	capabilities.SupportsDxil = capabilities.SupportsDxil && IsValid();
	capabilities.SupportsSpirV = capabilities.SupportsSpirV && IsValid();
	capabilities.SupportsDxilRayTracingLibrary = capabilities.SupportsDxilRayTracingLibrary && IsValid();
	capabilities.SupportsSpirVRayTracingLibrary = capabilities.SupportsSpirVRayTracingLibrary && IsValid();
	capabilities.SupportsDxilInlineRayQuery = capabilities.SupportsDxilInlineRayQuery && IsValid();
	capabilities.SupportsSpirVInlineRayQuery = capabilities.SupportsSpirVInlineRayQuery && IsValid();
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

CompiledShader DxcShaderBackend::Compile(const ShaderCompileRequest& request)
{
	if (!IsValid())
	{
		throw Diagnostics::Error("DXC backend is not initialized");
	}

	const std::string& sourceText = request.SourceCode;

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = sourceText.data();
	sourceBuffer.Size = sourceText.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	std::wstring wSourcePath = Strings::ToWide(std::string_view{request.VirtualSourcePath});
	std::wstring wEntryPoint = Strings::ToWide(std::string_view{request.EntryPoint});
	std::wstring wTargetProfile = Strings::ToWide(std::string_view{ShaderCompileProfile::BuildTargetProfile(request)});
	std::vector<std::wstring> wDefines;
	std::vector<LPCWSTR> args;

	BuildCompileArguments(request, wSourcePath, wEntryPoint, wTargetProfile, wDefines, args);

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
		throw Diagnostics::Error("DXC Compile() call failed");
	}

	std::string errorMsg = ExtractErrorMessage(result.Get());

	HRESULT status;
	result->GetStatus(&status);
	if (FAILED(status))
	{
		if (errorMsg.empty())
			errorMsg = "Compilation failed with no error message";
		SPDLOG_LOGGER_ERROR(g_dxcShaderBackendLogger, "Shader compilation failed: {}", errorMsg);
		throw Diagnostics::Error(std::move(errorMsg));
	}

	if (!errorMsg.empty())
	{
		SPDLOG_LOGGER_WARN(g_dxcShaderBackendLogger, "Shader warnings: {}", errorMsg);
	}

	std::vector<std::uint8_t> bytecode = ExtractBytecode(result.Get());
	if (bytecode.empty())
	{
		throw Diagnostics::Error("Failed to extract shader bytecode");
	}
	if (IsSpirVTarget(request.Target))
	{
		SpirVBindingNormalizer::Normalize(bytecode, request.DescriptorBindingRemaps);
	}

	// PDBs only meaningful for DXIL today; SPIR-V output does not produce a
	// DXC PDB blob, so SaveShaderSymbols returns an empty path harmlessly.
	const std::filesystem::path debugArtifactPath =
	    SaveShaderSymbols(result.Get(), request.SourceMounts.get().ResolvePhysicalPath(request.VirtualSourcePath));

	ShaderReflection reflection;
	if (request.UnitKind != ShaderCompileUnitKind::Library)
	{
		reflection = IsSpirVTarget(request.Target)
		    ? SpirVReflectionExtractor::Extract(bytecode, request.Stage)
		    : DxilReflectionExtractor::Extract(*m_utils.Get(), result.Get(), bytecode, request.Stage);
	}

	ShaderDebugArtifactSet debugArtifacts;
	if (request.CaptureDebugArtifacts)
	{
		debugArtifacts = CaptureDebugArtifacts(request, *m_utils.Get(), *m_compiler.Get(), sourceBuffer, bytecode, args, errorMsg);
	}

	CompiledShader compiledShader(std::move(bytecode), debugArtifactPath);
	compiledShader.SetReflection(std::move(reflection));
	compiledShader.SetDebugArtifacts(std::move(debugArtifacts));
	return compiledShader;
}

void DxcShaderBackend::BuildCompileArguments(
    const ShaderCompileRequest& request,
    const std::wstring& wSourcePath,
    const std::wstring& wEntryPoint,
    const std::wstring& wTargetProfile,
    std::vector<std::wstring>& wDefines,
    std::vector<LPCWSTR>& outArgs)
{
	outArgs.clear();
	outArgs.reserve(32);

	outArgs.push_back(wSourcePath.c_str());

	if (request.UnitKind != ShaderCompileUnitKind::Library)
	{
		outArgs.push_back(L"-E");
		outArgs.push_back(wEntryPoint.c_str());
	}

	outArgs.push_back(L"-T");
	outArgs.push_back(wTargetProfile.c_str());

	outArgs.push_back(L"-HV");
	outArgs.push_back(L"2021");

	wDefines.clear();
	for (const auto& def : request.Defines)
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

	if (request.TreatWarningsAsErrors)
	{
		outArgs.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
	}

	// Cooked packages serialize typed reflection separately, so runtime DXIL
	// does not need the embedded reflection payload. Both flags are DXIL-only.
	if (!IsSpirVTarget(request.Target))
	{
		outArgs.push_back(L"-Qstrip_reflect");
		if (request.StripDebugInfo)
		{
			outArgs.push_back(L"-Qstrip_debug");
		}
	}

	if (request.EnableDebugInfo)
	{
		outArgs.push_back(DXC_ARG_DEBUG);
	}

	if (request.EnableOptimizations)
	{
		outArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
	}
	else
	{
		outArgs.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
	}

	// SPIR-V mode toggle. DXC emits SPIR-V when -spirv is present; the same
	// HLSL profile string still selects the shader stage.
	if (IsSpirVTarget(request.Target))
	{
		outArgs.push_back(L"-spirv");
		outArgs.push_back(L"-fspv-use-unknown-image-format");
		switch (request.Target)
		{
			case ShaderTarget::SpirV14:
				outArgs.push_back(L"-fspv-target-env=vulkan1.1spirv1.4");
				break;
			case ShaderTarget::SpirV15:
				outArgs.push_back(L"-fspv-target-env=vulkan1.2");
				break;
			case ShaderTarget::SpirV16:
				outArgs.push_back(L"-fspv-target-env=vulkan1.3");
				break;
			default:
				break;
		}
		if (HasShaderCompileFeature(request.RequiredFeatures, ShaderCompileFeatureFlags::InlineRayQuery))
		{
			outArgs.push_back(L"-fspv-extension=SPV_KHR_ray_query");
		}
		if (HasShaderCompileFeature(request.RequiredFeatures, ShaderCompileFeatureFlags::AccelerationStructure))
		{
			outArgs.push_back(L"-fspv-extension=SPV_KHR_ray_tracing");
		}
		if (HasShaderCompileFeature(request.RequiredFeatures, ShaderCompileFeatureFlags::DescriptorIndexing))
		{
			outArgs.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
		}
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
	        IID_PPV_ARGS(preprocessResult.ReleaseAndGetAddressOf())))
	    || !preprocessResult)
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

std::string DxcShaderBackend::ExtractDisassembly(IDxcUtils& utils, IDxcCompiler3& compiler, std::span<const std::uint8_t> bytecode)
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
	if (FAILED(utils.CreateBlobFromPinned(bytecode.data(), static_cast<UINT32>(bytecode.size()), 0, objectBlob.ReleaseAndGetAddressOf()))
	    || !objectBlob)
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

ShaderDebugArtifactSet DxcShaderBackend::CaptureDebugArtifacts(
    const ShaderCompileRequest& request,
    IDxcUtils& utils,
    IDxcCompiler3& compiler,
    const DxcBuffer& sourceBuffer,
    std::span<const std::uint8_t> bytecode,
    const std::vector<LPCWSTR>& compileArgs,
    std::string_view compilerOutput)
{
	ShaderDebugArtifactSet debugArtifacts;
	debugArtifacts.CompileArguments = BuildDebugArgumentStrings(compileArgs);
	debugArtifacts.CompilerOutput.assign(compilerOutput);
	debugArtifacts.Disassembly = ExtractDisassembly(utils, compiler, bytecode);
	if (debugArtifacts.Disassembly.empty())
	{
		throw Diagnostics::Error("DXC failed to capture disassembly for shader source '" + request.VirtualSourcePath + "'.");
	}
	debugArtifacts.PreprocessedSource = ExtractPreprocessedSource(utils, compiler, sourceBuffer, compileArgs);
	if (debugArtifacts.PreprocessedSource.empty())
	{
		throw Diagnostics::Error("DXC failed to capture preprocessed source for shader source '" + request.VirtualSourcePath + "'.");
	}
	return debugArtifacts;
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
	const std::filesystem::path pdbPath = Filesystem::GetShaderSymbolsOutputPath() / std::filesystem::path(pdbName).filename();

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
