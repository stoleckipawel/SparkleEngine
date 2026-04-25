#include "PCH.h"

#include "Slang/SlangShaderBackend.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Slang/SlangReflectionExtractor.h"

#include <array>
#include <cstring>

SlangShaderBackend::SlangShaderBackend()
{
	if (SLANG_FAILED(slang::createGlobalSession(m_globalSession.writeRef())) || !m_globalSession)
	{
		m_backendVersion = Hash::Fnv1a64("slang-unavailable");
		return;
	}

	const char* buildTag = m_globalSession->getBuildTagString();
	m_backendVersion = Hash::Fnv1a64(std::string_view(buildTag != nullptr ? buildTag : "slang-unknown"));
}

ShaderBackendCapabilities SlangShaderBackend::GetCapabilities() const
{
	return ShaderBackendCapabilities{.SupportsDxil = IsValid(), .SupportsSpirV = IsValid()};
}

std::string_view SlangShaderBackend::GetBackendName() const
{
	return "slang";
}

std::uint64_t SlangShaderBackend::GetBackendVersion() const
{
	return m_backendVersion;
}

ShaderCompileResult SlangShaderBackend::Compile(const ShaderCompileOptions& options)
{
	if (!IsValid())
	{
		return ShaderCompileResult::Failure("Slang global session is unavailable");
	}

	std::string sourceError;
	const std::string sourceText = LoadSourceText(options.SourcePath, sourceError);
	if (!sourceError.empty())
	{
		return ShaderCompileResult::Failure(std::move(sourceError));
	}

	const std::string includeDir = options.IncludeDir.generic_string();
	std::vector<std::string> includeStorage;
	includeStorage.reserve(1 + options.AdditionalIncludeDirs.size());
	if (!includeDir.empty())
	{
		includeStorage.push_back(includeDir);
	}
	for (const std::filesystem::path& includePath : options.AdditionalIncludeDirs)
	{
		includeStorage.push_back(includePath.generic_string());
	}

	std::vector<const char*> includePaths;
	includePaths.reserve(includeStorage.size());
	for (const std::string& includePath : includeStorage)
	{
		includePaths.push_back(includePath.c_str());
	}

	std::vector<slang::PreprocessorMacroDesc> macroDescs;
	macroDescs.reserve(options.Defines.size());
	std::vector<std::string> defineNames;
	std::vector<std::string> defineValues;
	defineNames.reserve(options.Defines.size());
	defineValues.reserve(options.Defines.size());
	for (const std::string& define : options.Defines)
	{
		const std::size_t equals = define.find('=');
		defineNames.push_back(define.substr(0, equals));
		defineValues.push_back(equals != std::string::npos ? define.substr(equals + 1) : "1");
		macroDescs.push_back(slang::PreprocessorMacroDesc{defineNames.back().c_str(), defineValues.back().c_str()});
	}

	slang::TargetDesc targetDesc{};
	targetDesc.format = MapTarget(options.Target);
	targetDesc.profile = m_globalSession->findProfile(GetTargetProfileName(options.Target));
	if (targetDesc.format == SLANG_TARGET_UNKNOWN || targetDesc.profile == SLANG_PROFILE_UNKNOWN)
	{
		return ShaderCompileResult::Failure("Slang backend does not support requested shader target");
	}

	std::array<slang::CompilerOptionEntry, 1> spirvOptions = {{
	    {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}},
	}};
	if (IsSpirVTarget(options.Target))
	{
		targetDesc.compilerOptionEntries = spirvOptions.data();
		targetDesc.compilerOptionEntryCount = static_cast<std::uint32_t>(spirvOptions.size());
	}

	slang::SessionDesc sessionDesc{};
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.searchPaths = includePaths.data();
	sessionDesc.searchPathCount = static_cast<SlangInt>(includePaths.size());
	sessionDesc.preprocessorMacros = macroDescs.data();
	sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macroDescs.size());

	Slang::ComPtr<slang::ISession> session;
	if (SLANG_FAILED(m_globalSession->createSession(sessionDesc, session.writeRef())) || !session)
	{
		return ShaderCompileResult::Failure("Slang failed to create compile session");
	}

	std::string diagnostics;
	Slang::ComPtr<slang::IBlob> diagnosticBlob;
	const std::string moduleName = options.SourcePath.stem().generic_string();
	const std::string modulePath = options.SourcePath.generic_string();
	slang::IModule* module = session->loadModuleFromSourceString(
	    moduleName.c_str(),
	    modulePath.c_str(),
	    sourceText.c_str(),
	    diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (module == nullptr)
	{
		return ShaderCompileResult::Failure("Slang failed to load module - " + diagnostics);
	}

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	diagnosticBlob.setNull();
	const SlangStage stage = MapStage(options.Stage);
	SlangResult entryResult = module->findAndCheckEntryPoint(
	    options.EntryPoint.c_str(), stage, entryPoint.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(entryResult) || !entryPoint)
	{
		return ShaderCompileResult::Failure("Slang failed to find entry point - " + diagnostics);
	}

	std::array<slang::IComponentType*, 2> components = {module, entryPoint.get()};
	Slang::ComPtr<slang::IComponentType> composedProgram;
	diagnosticBlob.setNull();
	SlangResult composeResult = session->createCompositeComponentType(
	    components.data(), components.size(), composedProgram.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(composeResult) || !composedProgram)
	{
		return ShaderCompileResult::Failure("Slang failed to compose shader program - " + diagnostics);
	}

	Slang::ComPtr<slang::IComponentType> linkedProgram;
	diagnosticBlob.setNull();
	SlangResult linkResult = composedProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(linkResult) || !linkedProgram)
	{
		return ShaderCompileResult::Failure("Slang failed to link shader program - " + diagnostics);
	}

	Slang::ComPtr<slang::IBlob> codeBlob;
	diagnosticBlob.setNull();
	SlangResult codeResult = linkedProgram->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(codeResult) || !codeBlob || codeBlob->getBufferSize() == 0)
	{
		return ShaderCompileResult::Failure("Slang failed to emit target bytecode - " + diagnostics);
	}

	const auto* bytes = static_cast<const std::uint8_t*>(codeBlob->getBufferPointer());
	std::vector<std::uint8_t> bytecode(bytes, bytes + codeBlob->getBufferSize());
	ShaderCompileResult result = ShaderCompileResult::Success(std::move(bytecode));

	diagnosticBlob.setNull();
	slang::ProgramLayout* layout = linkedProgram->getLayout(0, diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (layout != nullptr)
	{
		ShaderReflection reflection;
		std::string reflectionError;
		if (SlangReflectionExtractor::Extract(*layout, options.Stage, reflection, reflectionError))
		{
			result.SetReflection(std::move(reflection));
		}
	}

	if (options.CaptureDebugArtifacts)
	{
		CaptureDebugArtifacts(options, sourceText, diagnostics, result);
	}

	return result;
}

SlangStage SlangShaderBackend::MapStage(ShaderStage stage)
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return SLANG_STAGE_VERTEX;
		case ShaderStage::Pixel:
			return SLANG_STAGE_FRAGMENT;
		case ShaderStage::Compute:
			return SLANG_STAGE_COMPUTE;
		default:
			return SLANG_STAGE_NONE;
	}
}

SlangCompileTarget SlangShaderBackend::MapTarget(ShaderTarget target)
{
	if (IsDxilTarget(target))
		return SLANG_DXIL;
	if (IsSpirVTarget(target))
		return SLANG_SPIRV;
	return SLANG_TARGET_UNKNOWN;
}

const char* SlangShaderBackend::GetTargetProfileName(ShaderTarget target)
{
	if (IsSpirVTarget(target))
		return "spirv_1_5";
	return "sm_6_0";
}

std::string SlangShaderBackend::BlobToString(slang::IBlob* blob)
{
	if (blob == nullptr || blob->getBufferPointer() == nullptr || blob->getBufferSize() == 0)
		return {};

	return std::string(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
}

std::string SlangShaderBackend::LoadSourceText(const std::filesystem::path& sourcePath, std::string& outErrorMessage)
{
	std::vector<std::uint8_t> bytes;
	if (!Engine::Files::TryReadAllBytes(sourcePath, bytes, outErrorMessage))
	{
		outErrorMessage = "Failed to read shader source for Slang backend '" + sourcePath.generic_string() + "' - " + outErrorMessage;
		return {};
	}

	outErrorMessage.clear();
	return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::vector<std::string> SlangShaderBackend::BuildDebugArgumentStrings(const ShaderCompileOptions& options)
{
	std::vector<std::string> args;
	args.push_back("slang-api");
	args.push_back("-entry");
	args.push_back(options.EntryPoint);
	args.push_back("-target");
	args.push_back(IsSpirVTarget(options.Target) ? "spirv" : "dxil");
	args.push_back("-profile");
	args.push_back(GetTargetProfileName(options.Target));
	return args;
}

void SlangShaderBackend::CaptureDebugArtifacts(
    const ShaderCompileOptions& options,
    std::string_view sourceText,
    std::string_view diagnostics,
    ShaderCompileResult& outCompileResult)
{
	ShaderDebugArtifactSet artifacts;
	artifacts.CompileArguments = BuildDebugArgumentStrings(options);
	artifacts.CompilerOutput.assign(diagnostics);
	artifacts.PreprocessedSource.assign(sourceText);
	artifacts.Disassembly = "Slang backend disassembly is not captured yet.";
	outCompileResult.SetDebugArtifacts(std::move(artifacts));
}