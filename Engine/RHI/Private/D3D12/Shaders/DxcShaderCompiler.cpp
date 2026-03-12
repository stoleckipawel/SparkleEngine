#include "PCH.h"
#include "DxcShaderCompiler.h"
#include "DxcContext.h"
#include "Assets/AssetSystem.h"
#include "Strings/StringUtils.h"

ShaderCompileResult DxcShaderCompiler::CompileFromAsset(
    const AssetSystem& assetSystem,
    const std::filesystem::path& sourcePath,
    ShaderStage stage,
    const std::string& entryPoint)
{
	const auto resolvedPath = assetSystem.ResolvePathValidated(sourcePath, AssetType::Shader);

	ShaderCompileOptions options;
	options.SourcePath = resolvedPath;
	options.EntryPoint = entryPoint;
	options.Stage = stage;

	ConfigureIncludePaths(assetSystem, options);
	ApplyBuildConfiguration(options);

	LOG_INFO("Compiling shader: " + resolvedPath.string());
	return Compile(assetSystem, options);
}

void DxcShaderCompiler::ConfigureIncludePaths(const AssetSystem& assetSystem, ShaderCompileOptions& options)
{
	options.AdditionalIncludeDirs.clear();

	const auto& projectShaders = assetSystem.GetTypedPath(AssetType::Shader, PathRoot::Project);
	const auto& engineShaders = assetSystem.GetTypedPath(AssetType::Shader, PathRoot::Engine);

	options.IncludeDir = !projectShaders.empty() ? projectShaders : engineShaders;

	if (!projectShaders.empty() && !engineShaders.empty())
	{
		options.AdditionalIncludeDirs.push_back(engineShaders);
	}
}

void DxcShaderCompiler::ApplyBuildConfiguration(ShaderCompileOptions& options)
{
#if defined(ENGINE_SHADERS_DEBUG)
	options.EnableDebugInfo = true;
#endif

#if defined(ENGINE_SHADERS_OPTIMIZED)
	options.EnableOptimizations = true;
#else
	options.EnableOptimizations = false;
#endif
}

ShaderCompileResult DxcShaderCompiler::Compile(const AssetSystem& assetSystem, const ShaderCompileOptions& options)
{
	DxcContext& ctx = GetDxcContext();
	if (!ctx.IsValid())
	{
		return ShaderCompileResult::Failure("DXC context is not initialized");
	}

	ComPtr<IDxcBlobEncoding> sourceBlob;
	HRESULT hr = ctx.GetUtils()->LoadFile(options.SourcePath.c_str(), nullptr, sourceBlob.ReleaseAndGetAddressOf());
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

	ComPtr<IDxcIncludeHandler> includeHandler = ctx.CreateIncludeHandler();

	ComPtr<IDxcResult> result;
	hr = ctx.GetCompiler()->Compile(
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
		LOG_FATAL("Shader compilation failed: " + errorMsg);
		return ShaderCompileResult::Failure(std::move(errorMsg));
	}

	if (!errorMsg.empty())
	{
		LOG_WARNING("Shader warnings: " + errorMsg);
	}

	std::vector<uint8_t> bytecode = ExtractBytecode(result.Get());
	if (bytecode.empty())
	{
		return ShaderCompileResult::Failure("Failed to extract shader bytecode");
	}

	SaveShaderSymbols(assetSystem, result.Get(), options.SourcePath);

	LOG_INFO("Shader compiled successfully: " + options.SourcePath.filename().string());
	return ShaderCompileResult::Success(std::move(bytecode));
}

void DxcShaderCompiler::BuildCompileArguments(
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

	if (options.StripReflection)
	{
		outArgs.push_back(L"-Qstrip_reflect");
	}
	if (options.StripDebugInfo)
	{
		outArgs.push_back(L"-Qstrip_debug");
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
}

std::vector<uint8_t> DxcShaderCompiler::ExtractBytecode(IDxcResult* result)
{
	ComPtr<IDxcBlob> shaderBlob;
	HRESULT hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.ReleaseAndGetAddressOf()), nullptr);
	if (FAILED(hr) || !shaderBlob || shaderBlob->GetBufferSize() == 0)
	{
		return {};
	}

	const uint8_t* data = static_cast<const uint8_t*>(shaderBlob->GetBufferPointer());
	return std::vector<uint8_t>(data, data + shaderBlob->GetBufferSize());
}

std::string DxcShaderCompiler::ExtractErrorMessage(IDxcResult* result)
{
	ComPtr<IDxcBlobUtf8> errorBlob;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errorBlob.ReleaseAndGetAddressOf()), nullptr);
	if (errorBlob && errorBlob->GetStringLength() > 0)
	{
		return std::string(errorBlob->GetStringPointer(), errorBlob->GetStringLength());
	}
	return {};
}

void DxcShaderCompiler::SaveShaderSymbols(const AssetSystem& assetSystem, IDxcResult* result, const std::filesystem::path& sourcePath)
{
	ComPtr<IDxcBlob> pdbBlob;
	ComPtr<IDxcBlobUtf16> pdbNameBlob;
	result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdbBlob.ReleaseAndGetAddressOf()), pdbNameBlob.ReleaseAndGetAddressOf());

	if (!pdbBlob || !pdbNameBlob)
		return;

	const std::filesystem::path& symbolsDir = assetSystem.GetShaderSymbolsOutputPath();
	std::wstring pdbName(pdbNameBlob->GetStringPointer());
	const std::filesystem::path pdbFilename = std::filesystem::path(pdbName).filename();
	const std::filesystem::path pdbPath = symbolsDir / pdbFilename;

	FILE* fp = nullptr;
	_wfopen_s(&fp, pdbPath.c_str(), L"wb");
	if (fp)
	{
		fwrite(pdbBlob->GetBufferPointer(), 1, pdbBlob->GetBufferSize(), fp);
		fclose(fp);
	}
}