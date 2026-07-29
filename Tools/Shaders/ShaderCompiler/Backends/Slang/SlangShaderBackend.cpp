#include "PCH.h"

#include "Slang/SlangShaderBackend.h"

#include "Compiler/ShaderCompileProfile.h"
#include "Compiler/ShaderCompilerPaths.h"
#include "Compiler/ShaderSourcePreprocessor.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Slang/SlangReflectionExtractor.h"

#include <array>

SlangShaderBackend::SlangShaderBackend()
{
	if (SLANG_FAILED(slang::createGlobalSession(m_globalSession.writeRef())) || !m_globalSession)
	{
		m_backendVersion = Hash::Fnv1a64("slang-unavailable");
		return;
	}

	m_backendVersion = QueryBackendVersion(*m_globalSession.get());
}

ShaderBackendCapabilities SlangShaderBackend::GetStaticCapabilities() noexcept
{
	return ShaderBackendCapabilities{.SupportsDxil = true, .SupportsSpirV = true};
}

std::uint64_t SlangShaderBackend::QueryBackendVersion()
{
	Slang::ComPtr<slang::IGlobalSession> globalSession;
	if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())) || !globalSession)
	{
		return Hash::Fnv1a64("slang-unavailable");
	}
	return QueryBackendVersion(*globalSession.get());
}

std::uint64_t SlangShaderBackend::QueryBackendVersion(slang::IGlobalSession& globalSession)
{
	const char* buildTag = globalSession.getBuildTagString();
	return Hash::Fnv1a64(std::string_view(buildTag != nullptr ? buildTag : "slang-unknown"));
}

ShaderBackendCapabilities SlangShaderBackend::GetCapabilities() const
{
	ShaderBackendCapabilities capabilities = GetStaticCapabilities();
	capabilities.SupportsDxil = capabilities.SupportsDxil && IsValid();
	capabilities.SupportsSpirV = capabilities.SupportsSpirV && IsValid();
	return capabilities;
}

std::string_view SlangShaderBackend::GetBackendName() const
{
	return "slang";
}

std::uint64_t SlangShaderBackend::GetBackendVersion() const
{
	return m_backendVersion;
}

CompiledShader SlangShaderBackend::Compile(const ShaderCompileOptions& options)
{
	if (!IsValid())
	{
		throw Diagnostics::Error("Slang global session is unavailable");
	}

	const std::filesystem::path sourcePath = ShaderCompilerPaths::CanonicalizeForCompiler(options.SourcePath);
	const std::string sourceText = ShaderSourcePreprocessor::Load(sourcePath, options);

	const std::string includeDir = ShaderCompilerPaths::MakeIncludeDirectoryArgument(options.IncludeDir);
	std::vector<std::string> includeStorage;
	includeStorage.reserve(1 + options.AdditionalIncludeDirs.size());
	if (!includeDir.empty())
	{
		includeStorage.push_back(includeDir);
	}
	for (const std::filesystem::path& includePath : options.AdditionalIncludeDirs)
	{
		includeStorage.push_back(ShaderCompilerPaths::MakeIncludeDirectoryArgument(includePath));
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
	targetDesc.profile = m_globalSession->findProfile(ShaderCompileProfile::GetSlangTargetProfileName(options.Target));
	if (targetDesc.format == SLANG_TARGET_UNKNOWN || targetDesc.profile == SLANG_PROFILE_UNKNOWN)
	{
		throw Diagnostics::Error("Slang backend does not support requested shader target");
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
		throw Diagnostics::Error("Slang failed to create compile session");
	}

	std::string diagnostics;
	Slang::ComPtr<slang::IBlob> diagnosticBlob;
	const std::string moduleName = sourcePath.stem().generic_string();
	const std::string modulePath = sourcePath.generic_string();
	slang::IModule* module = session->loadModuleFromSourceString(
	    moduleName.c_str(),
	    modulePath.c_str(),
	    sourceText.c_str(),
	    diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (module == nullptr)
	{
		throw Diagnostics::Error("Slang failed to load module - " + diagnostics);
	}

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	diagnosticBlob.setNull();
	const SlangStage stage = MapStage(options.Stage);
	SlangResult entryResult = module->findAndCheckEntryPoint(
	    options.EntryPoint.c_str(), stage, entryPoint.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(entryResult) || !entryPoint)
	{
		throw Diagnostics::Error("Slang failed to find entry point - " + diagnostics);
	}

	std::array<slang::IComponentType*, 2> components = {module, entryPoint.get()};
	Slang::ComPtr<slang::IComponentType> composedProgram;
	diagnosticBlob.setNull();
	SlangResult composeResult = session->createCompositeComponentType(
	    components.data(), components.size(), composedProgram.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(composeResult) || !composedProgram)
	{
		throw Diagnostics::Error("Slang failed to compose shader program - " + diagnostics);
	}

	Slang::ComPtr<slang::IComponentType> linkedProgram;
	diagnosticBlob.setNull();
	SlangResult linkResult = composedProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(linkResult) || !linkedProgram)
	{
		throw Diagnostics::Error("Slang failed to link shader program - " + diagnostics);
	}

	Slang::ComPtr<slang::IBlob> codeBlob;
	diagnosticBlob.setNull();
	SlangResult codeResult = linkedProgram->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (SLANG_FAILED(codeResult) || !codeBlob || codeBlob->getBufferSize() == 0)
	{
		throw Diagnostics::Error("Slang failed to emit target bytecode - " + diagnostics);
	}

	const auto* bytes = static_cast<const std::uint8_t*>(codeBlob->getBufferPointer());
	std::vector<std::uint8_t> bytecode(bytes, bytes + codeBlob->getBufferSize());

	diagnosticBlob.setNull();
	slang::ProgramLayout* layout = linkedProgram->getLayout(0, diagnosticBlob.writeRef());
	diagnostics += BlobToString(diagnosticBlob);
	if (layout == nullptr)
	{
		throw Diagnostics::Error(
		    "Slang failed to produce reflection layout for target '" + std::string{GetShaderTargetName(options.Target)} +
		    "' source '" + options.SourcePath.generic_string() + "' entry '" + options.EntryPoint + "' - " + diagnostics);
	}

	ShaderReflection reflection = SlangReflectionExtractor::Extract(*layout, options.Stage);

	ShaderDebugArtifactSet debugArtifacts;
	if (options.CaptureDebugArtifacts)
	{
		debugArtifacts = CaptureDebugArtifacts(options, sourceText, diagnostics);
	}

	CompiledShader compiledShader(std::move(bytecode));
	compiledShader.SetReflection(std::move(reflection));
	compiledShader.SetDebugArtifacts(std::move(debugArtifacts));
	return compiledShader;
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

std::string SlangShaderBackend::BlobToString(slang::IBlob* blob)
{
	if (blob == nullptr || blob->getBufferPointer() == nullptr || blob->getBufferSize() == 0)
		return {};

	return std::string(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
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
	args.push_back(ShaderCompileProfile::GetSlangTargetProfileName(options.Target));
	return args;
}

ShaderDebugArtifactSet SlangShaderBackend::CaptureDebugArtifacts(
    const ShaderCompileOptions& options,
    std::string_view sourceText,
    std::string_view diagnostics)
{
	ShaderDebugArtifactSet artifacts;
	artifacts.CompileArguments = BuildDebugArgumentStrings(options);
	artifacts.CompilerOutput.assign(diagnostics);
	artifacts.PreprocessedSource.assign(sourceText);
	return artifacts;
}
