#include "PCH.h"

#include "Cooking/ShaderDebugArtifactWriter.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <format>
#include <sstream>

bool ShaderDebugArtifactWriter::Write(
	const std::filesystem::path& rootDirectory,
	const ShaderCookPackageDesc& package,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	const CookedStageBuild& compiledStage,
	const ShaderDebugArtifactSet& debugArtifacts,
	std::string& outErrorMessage)
{
	const std::filesystem::path bundleDirectory = rootDirectory / BuildBundleDirectoryName(package, stage, options, compiledStage);

	if (!Files::TryWriteAllText(bundleDirectory / "compile-request.json", BuildCompileRequestJson(package, stage, options, compiledStage), outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "cache-info.json", BuildCacheInfoJson(options, compiledStage), outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "defines.json", BuildDefinesJson(options), outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "permutation-vector.json", BuildPermutationJson(package), outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "preprocessed-source.hlsl", debugArtifacts.PreprocessedSource, outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "reflection.json", BuildReflectionJson(compiledStage.reflection), outErrorMessage) ||
		!Files::TryWriteAllText(
			bundleDirectory / "parameter-struct-match.json",
			debugArtifacts.ParameterMatchReportJson.empty() ? BuildParameterMatchJson() : debugArtifacts.ParameterMatchReportJson,
			outErrorMessage) ||
		!Files::TryWriteAllText(
			bundleDirectory / "disassembly.txt",
			debugArtifacts.Disassembly.empty()
				? std::string_view{"Disassembly capture unavailable for this backend/target in the current environment.\n"}
				: std::string_view{debugArtifacts.Disassembly},
			outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "compiler-stderr.txt", debugArtifacts.CompilerOutput, outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "compile-args.json", BuildCompileArgsJson(debugArtifacts), outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

std::string ShaderDebugArtifactWriter::BuildBundleDirectoryName(
	const ShaderCookPackageDesc& package,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	const CookedStageBuild& compiledStage)
{
	const std::string shaderId = Paths::MakeSafePathComponent(package.packageId + "_" + std::string(GetShaderStagePrefix(stage.stage)));
	const std::uint64_t permutationHash = Hash::Fnv1a64(package.variantId);
	return std::format(
	    "{}__{:016x}__{}__{}",
	    shaderId,
	    permutationHash,
	    Paths::MakeSafePathComponent(compiledStage.backendName),
	    Paths::MakeSafePathComponent(GetShaderTargetName(options.Target)));
}

std::string ShaderDebugArtifactWriter::BuildCompileRequestJson(
	const ShaderCookPackageDesc& package,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	const CookedStageBuild& compiledStage)
{
	return std::format(
	    "{{\n"
	    "  \"packageId\": \"{}\",\n"
	    "  \"variantId\": \"{}\",\n"
	    "  \"bindingLayoutId\": \"{}\",\n"
	    "  \"sourcePath\": \"{}\",\n"
	    "  \"entryPoint\": \"{}\",\n"
	    "  \"stage\": \"{}\",\n"
	    "  \"target\": \"{}\",\n"
	    "  \"backend\": \"{}\",\n"
	    "  \"backendVersion\": {},\n"
	    "  \"format\": \"{}\",\n"
	    "  \"sourceHash\": \"{:016x}\",\n"
	    "  \"includeClosureHash\": \"{:016x}\",\n"
	    "  \"optionsHash\": \"{:016x}\",\n"
	    "  \"cacheKey\": \"{:016x}\",\n"
	    "  \"cacheStatus\": \"{}\",\n"
	    "  \"debugArtifact\": \"{}\"\n"
	    "}}\n",
	    Strings::EscapeJsonString(package.packageId),
	    Strings::EscapeJsonString(package.variantId),
	    Strings::EscapeJsonString(package.bindingLayoutId),
	    Strings::EscapeJsonString(stage.sourcePath.generic_string()),
	    Strings::EscapeJsonString(stage.entryPoint),
	    Strings::EscapeJsonString(GetShaderStagePrefix(stage.stage)),
	    Strings::EscapeJsonString(GetShaderTargetName(options.Target)),
	    Strings::EscapeJsonString(compiledStage.backendName),
	    compiledStage.backendVersion,
	    compiledStage.format == CookedShaderBinaryFormat::SpirV ? "SpirV" : "Dxil",
	    compiledStage.sourceHash,
	    compiledStage.includeClosureHash,
	    compiledStage.optionsHash,
	    compiledStage.cacheKey,
	    Strings::EscapeJsonString(compiledStage.cacheStatus),
	    Strings::EscapeJsonString(compiledStage.debugArtifact));
}

std::string ShaderDebugArtifactWriter::BuildCacheInfoJson(const ShaderCompileOptions& options, const CookedStageBuild& compiledStage)
{
	return std::format(
	    "{{\n"
	    "  \"cacheStatus\": \"{}\",\n"
	    "  \"cacheKey\": \"{:016x}\",\n"
	    "  \"sourceHash\": \"{:016x}\",\n"
	    "  \"includeClosureHash\": \"{:016x}\",\n"
	    "  \"optionsHash\": \"{:016x}\",\n"
	    "  \"target\": \"{}\",\n"
	    "  \"format\": \"{}\",\n"
	    "  \"backend\": \"{}\",\n"
	    "  \"backendVersion\": {}\n"
	    "}}\n",
	    Strings::EscapeJsonString(compiledStage.cacheStatus),
	    compiledStage.cacheKey,
	    compiledStage.sourceHash,
	    compiledStage.includeClosureHash,
	    compiledStage.optionsHash,
	    Strings::EscapeJsonString(GetShaderTargetName(options.Target)),
	    compiledStage.format == CookedShaderBinaryFormat::SpirV ? "SpirV" : "Dxil",
	    Strings::EscapeJsonString(compiledStage.backendName),
	    compiledStage.backendVersion);
}

std::string ShaderDebugArtifactWriter::BuildDefinesJson(const ShaderCompileOptions& options)
{
	std::ostringstream stream;
	stream << "[\n";
	for (std::size_t index = 0; index < options.Defines.size(); ++index)
	{
		stream << "  \"" << Strings::EscapeJsonString(options.Defines[index]) << "\"";
		if (index + 1 < options.Defines.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "]\n";
	return stream.str();
}

std::string ShaderDebugArtifactWriter::BuildPermutationJson(const ShaderCookPackageDesc& package)
{
	return std::format(
	    "{{\n"
	    "  \"variantId\": \"{}\",\n"
	    "  \"variantHash\": \"{:016x}\"\n"
	    "}}\n",
	    Strings::EscapeJsonString(package.variantId),
	    Hash::Fnv1a64(package.variantId));
}

std::string ShaderDebugArtifactWriter::BuildCompileArgsJson(const ShaderDebugArtifactSet& debugArtifacts)
{
	std::ostringstream stream;
	stream << "[\n";
	for (std::size_t index = 0; index < debugArtifacts.CompileArguments.size(); ++index)
	{
		stream << "  \"" << Strings::EscapeJsonString(debugArtifacts.CompileArguments[index]) << "\"";
		if (index + 1 < debugArtifacts.CompileArguments.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "]\n";
	return stream.str();
}

std::string ShaderDebugArtifactWriter::BuildReflectionJson(const ShaderReflection& reflection)
{
	std::ostringstream stream;
	stream << "{\n";
	stream << "  \"threadGroupSize\": [" << reflection.ThreadGroupSize[0] << ", " << reflection.ThreadGroupSize[1] << ", "
	       << reflection.ThreadGroupSize[2] << "],\n";
	stream << "  \"entryFlags\": " << reflection.EntryFlags << ",\n";
	stream << "  \"waveSize\": " << reflection.WaveSize << ",\n";
	stream << "  \"bindings\": [\n";
	for (std::size_t index = 0; index < reflection.Bindings.size(); ++index)
	{
		const ShaderReflectionResourceBinding& binding = reflection.Bindings[index];
		stream << std::format(
		    "    {{ \"name\": \"{}\", \"set\": {}, \"slot\": {}, \"arrayCount\": {}, \"sizeInBytes\": {}, \"readOnly\": {}, \"constantBufferIndex\": {} }}",
		    Strings::EscapeJsonString(binding.Name),
		    binding.Set,
		    binding.Slot,
		    binding.ArrayCount,
		    binding.SizeInBytes,
		    binding.IsReadOnly ? "true" : "false",
		    binding.ConstantBufferIndex);
		if (index + 1 < reflection.Bindings.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ],\n";
	stream << "  \"constantBuffers\": [\n";
	for (std::size_t index = 0; index < reflection.ConstantBuffers.size(); ++index)
	{
		const ShaderReflectionConstantBuffer& cb = reflection.ConstantBuffers[index];
		stream << std::format(
		    "    {{ \"name\": \"{}\", \"sizeInBytes\": {}, \"memberCount\": {} }}",
		    Strings::EscapeJsonString(cb.Name),
		    cb.SizeInBytes,
		    cb.Members.size());
		if (index + 1 < reflection.ConstantBuffers.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ],\n";
	stream << "  \"inputElements\": [\n";
	for (std::size_t index = 0; index < reflection.InputElements.size(); ++index)
	{
		const ShaderReflectionInputElement& input = reflection.InputElements[index];
		stream << std::format(
		    "    {{ \"semantic\": \"{}\", \"semanticIndex\": {}, \"location\": {}, \"componentCount\": {} }}",
		    Strings::EscapeJsonString(input.Semantic),
		    input.SemanticIndex,
		    input.Location,
		    input.ComponentCount);
		if (index + 1 < reflection.InputElements.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ],\n";
	stream << "  \"pushConstants\": [\n";
	for (std::size_t index = 0; index < reflection.PushConstants.size(); ++index)
	{
		const ShaderReflectionPushConstantRange& range = reflection.PushConstants[index];
		stream << std::format(
		    "    {{ \"offsetInBytes\": {}, \"sizeInBytes\": {}, \"visibilityMask\": {} }}",
		    range.OffsetInBytes,
		    range.SizeInBytes,
		    static_cast<unsigned>(range.VisibilityMask));
		if (index + 1 < reflection.PushConstants.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ],\n";
	stream << "  \"specializationConstants\": [\n";
	for (std::size_t index = 0; index < reflection.SpecializationConstants.size(); ++index)
	{
		const ShaderReflectionSpecializationConstant& spec = reflection.SpecializationConstants[index];
		stream << std::format(
		    "    {{ \"name\": \"{}\", \"constantId\": {}, \"defaultValueBits\": {} }}",
		    Strings::EscapeJsonString(spec.Name),
		    spec.ConstantId,
		    spec.DefaultValueBits);
		if (index + 1 < reflection.SpecializationConstants.size())
		{
			stream << ',';
		}
		stream << "\n";
	}
	stream << "  ]\n";
	stream << "}\n";
	return stream.str();
}

std::string ShaderDebugArtifactWriter::BuildParameterMatchJson()
{
	return "{\n  \"status\": \"not-run\",\n  \"reason\": \"ShaderParameterStructVerifier lands in Phase 3\"\n}\n";
}
