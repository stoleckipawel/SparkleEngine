#include "PCH.h"

#include "Cooking/ShaderDebugArtifactWriter.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

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
		!Files::TryWriteAllText(bundleDirectory / "defines.json", Json::WriteStringArray(options.Defines), outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "preprocessed-source.hlsl", debugArtifacts.PreprocessedSource, outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "reflection.json", BuildReflectionJson(compiledStage.reflection), outErrorMessage) ||
		!Files::TryWriteAllText(
			bundleDirectory / "parameter-struct-match.json",
			debugArtifacts.ParameterMatchReportJson,
			outErrorMessage) ||
		!Files::TryWriteAllText(
			bundleDirectory / "disassembly.txt",
			debugArtifacts.Disassembly.empty()
				? std::string_view{"Disassembly capture unavailable for this backend/target in the current environment.\n"}
				: std::string_view{debugArtifacts.Disassembly},
			outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "compiler-stderr.txt", debugArtifacts.CompilerOutput, outErrorMessage) ||
		!Files::TryWriteAllText(bundleDirectory / "compile-args.json", Json::WriteStringArray(debugArtifacts.CompileArguments), outErrorMessage))
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
	return std::format(
	    "{}__{}__{}__{}",
	    shaderId,
	    Formatting::FormatHexUInt64(BuildShaderPackageKey(package.packageId)),
	    Paths::MakeSafePathComponent(compiledStage.backendName),
	    Paths::MakeSafePathComponent(GetShaderTargetName(options.Target)));
}

std::string ShaderDebugArtifactWriter::BuildCompileRequestJson(
	const ShaderCookPackageDesc& package,
	const ShaderCookStageDesc& stage,
	const ShaderCompileOptions& options,
	const CookedStageBuild& compiledStage)
{
	Json::ObjectWriter writer;
	writer.WriteString("packageId", package.packageId);
	writer.WriteString("bindingLayoutId", package.bindingLayoutId);
	writer.WriteString("sourcePath", stage.sourcePath.generic_string());
	writer.WriteString("entryPoint", stage.entryPoint);
	writer.WriteString("stage", GetShaderStagePrefix(stage.stage));
	writer.WriteString("target", GetShaderTargetName(options.Target));
	writer.WriteString("backend", compiledStage.backendName);
	writer.WriteUInt64("backendVersion", compiledStage.backendVersion);
	writer.WriteString("format", compiledStage.format == CookedShaderBinaryFormat::SpirV ? "SpirV" : "Dxil");
	writer.WriteHexUInt64("sourceHash", compiledStage.sourceHash);
	writer.WriteHexUInt64("includeClosureHash", compiledStage.includeClosureHash);
	writer.WriteHexUInt64("optionsHash", compiledStage.optionsHash);
	writer.WriteHexUInt64("cacheKey", compiledStage.cacheKey);
	writer.WriteString("cacheStatus", compiledStage.cacheStatus);
	writer.WriteString("debugArtifact", compiledStage.debugArtifact);
	return writer.Finish();
}

std::string ShaderDebugArtifactWriter::BuildCacheInfoJson(const ShaderCompileOptions& options, const CookedStageBuild& compiledStage)
{
	Json::ObjectWriter writer;
	writer.WriteString("cacheStatus", compiledStage.cacheStatus);
	writer.WriteHexUInt64("cacheKey", compiledStage.cacheKey);
	writer.WriteHexUInt64("sourceHash", compiledStage.sourceHash);
	writer.WriteHexUInt64("includeClosureHash", compiledStage.includeClosureHash);
	writer.WriteHexUInt64("optionsHash", compiledStage.optionsHash);
	writer.WriteString("target", GetShaderTargetName(options.Target));
	writer.WriteString("format", compiledStage.format == CookedShaderBinaryFormat::SpirV ? "SpirV" : "Dxil");
	writer.WriteString("backend", compiledStage.backendName);
	writer.WriteUInt64("backendVersion", compiledStage.backendVersion);
	return writer.Finish();
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
		    "    {{ \"name\": {}, \"set\": {}, \"slot\": {}, \"arrayCount\": {}, \"sizeInBytes\": {}, \"readOnly\": {}, \"constantBufferIndex\": {} }}",
		    Json::QuoteString(binding.Name),
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
		    "    {{ \"name\": {}, \"sizeInBytes\": {}, \"memberCount\": {} }}",
		    Json::QuoteString(cb.Name),
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
		    "    {{ \"semantic\": {}, \"semanticIndex\": {}, \"location\": {}, \"componentCount\": {} }}",
		    Json::QuoteString(input.Semantic),
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
		    "    {{ \"name\": {}, \"constantId\": {}, \"defaultValueBits\": {} }}",
		    Json::QuoteString(spec.Name),
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
