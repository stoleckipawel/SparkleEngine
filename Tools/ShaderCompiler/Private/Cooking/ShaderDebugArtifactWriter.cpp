#include "PCH.h"

#include "Cooking/ShaderDebugArtifactWriter.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"

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

	if (!WriteTextFile(bundleDirectory / "compile-request.json", BuildCompileRequestJson(package, stage, options, compiledStage), outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "defines.json", BuildDefinesJson(options), outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "permutation-vector.json", BuildPermutationJson(package), outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "preprocessed-source.hlsl", debugArtifacts.PreprocessedSource, outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "reflection.json", BuildReflectionJson(compiledStage.reflection), outErrorMessage) ||
		!WriteTextFile(
			bundleDirectory / "parameter-struct-match.json",
			debugArtifacts.ParameterMatchReportJson.empty() ? BuildParameterMatchJson() : debugArtifacts.ParameterMatchReportJson,
			outErrorMessage) ||
		!WriteTextFile(
			bundleDirectory / "disassembly.txt",
			debugArtifacts.Disassembly.empty()
				? std::string_view{"Disassembly capture unavailable for this backend/target in the current environment.\n"}
				: std::string_view{debugArtifacts.Disassembly},
			outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "compiler-stderr.txt", debugArtifacts.CompilerOutput, outErrorMessage) ||
		!WriteTextFile(bundleDirectory / "compile-args.json", BuildCompileArgsJson(debugArtifacts), outErrorMessage))
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
	const std::string shaderId = SanitizePathComponent(package.packageId + "_" + std::string(GetShaderStagePrefix(stage.stage)));
	const std::uint64_t permutationHash = Hash::Fnv1a64(package.variantId);
	return std::format(
	    "{}__{:016x}__{}__{}",
	    shaderId,
	    permutationHash,
	    SanitizePathComponent(compiledStage.backendName),
	    SanitizePathComponent(GetShaderTargetName(options.Target)));
}

std::string ShaderDebugArtifactWriter::SanitizePathComponent(std::string_view value)
{
	std::string result;
	result.reserve(value.size());
	for (const char ch : value)
	{
		const bool allowed = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
		result.push_back(allowed ? ch : '_');
	}
	return result;
}

std::string ShaderDebugArtifactWriter::EscapeJson(std::string_view value)
{
	std::string result;
	result.reserve(value.size() + 8);
	for (const char ch : value)
	{
		switch (ch)
		{
			case '\\': result += "\\\\"; break;
			case '"': result += "\\\""; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '\t': result += "\\t"; break;
			default: result.push_back(ch); break;
		}
	}
	return result;
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
	    "  \"debugArtifact\": \"{}\"\n"
	    "}}\n",
	    EscapeJson(package.packageId),
	    EscapeJson(package.variantId),
	    EscapeJson(package.bindingLayoutId),
	    EscapeJson(stage.sourcePath.generic_string()),
	    EscapeJson(stage.entryPoint),
	    EscapeJson(GetShaderStagePrefix(stage.stage)),
	    EscapeJson(GetShaderTargetName(options.Target)),
	    EscapeJson(compiledStage.backendName),
	    compiledStage.backendVersion,
	    compiledStage.format == CookedShaderBinaryFormat::SpirV ? "SpirV" : "Dxil",
	    EscapeJson(compiledStage.debugArtifact));
}

std::string ShaderDebugArtifactWriter::BuildDefinesJson(const ShaderCompileOptions& options)
{
	std::ostringstream stream;
	stream << "[\n";
	for (std::size_t index = 0; index < options.Defines.size(); ++index)
	{
		stream << "  \"" << EscapeJson(options.Defines[index]) << "\"";
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
	    EscapeJson(package.variantId),
	    Hash::Fnv1a64(package.variantId));
}

std::string ShaderDebugArtifactWriter::BuildCompileArgsJson(const ShaderDebugArtifactSet& debugArtifacts)
{
	std::ostringstream stream;
	stream << "[\n";
	for (std::size_t index = 0; index < debugArtifacts.CompileArguments.size(); ++index)
	{
		stream << "  \"" << EscapeJson(debugArtifacts.CompileArguments[index]) << "\"";
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
		    EscapeJson(binding.Name),
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
		    EscapeJson(cb.Name),
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
		    EscapeJson(input.Semantic),
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
		    EscapeJson(spec.Name),
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

bool ShaderDebugArtifactWriter::WriteTextFile(
	const std::filesystem::path& path,
	std::string_view text,
	std::string& outErrorMessage)
{
	return Files::TryWriteAllText(path, text, outErrorMessage);
}