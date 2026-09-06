#include "PCH.h"

#include "Cooking/ShaderDebugArtifactWriter.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/PathUtils.h"

#include <format>
#include <sstream>

void ShaderDebugArtifactWriter::WriteText(const std::filesystem::path& path, std::string_view contents)
{
	std::string fileError;
	if (!Files::TryWriteAllText(path, contents, fileError))
	{
		throw Diagnostics::Error(fileError);
	}
}

void ShaderDebugArtifactWriter::Write(
    const std::filesystem::path& rootDirectory,
    const ShaderCompileRequest& request,
    const CookedStageBuild& compiledStage,
    const ShaderDebugArtifactSet& debugArtifacts)
{
	const std::filesystem::path bundleDirectory = rootDirectory / BuildBundleDirectoryName(request, compiledStage);
	WriteCompileInputs(bundleDirectory, request, compiledStage);
	WriteCompilerOutputs(bundleDirectory, debugArtifacts, compiledStage);
}

void ShaderDebugArtifactWriter::WriteCompileInputs(
    const std::filesystem::path& bundleDirectory,
    const ShaderCompileRequest& request,
    const CookedStageBuild& compiledStage)
{
	WriteText(bundleDirectory / "compile-request.json", BuildCompileRequestJson(request, compiledStage));
	WriteText(bundleDirectory / "compile-identity.json", BuildCompileIdentityJson(request, compiledStage));
	WriteText(bundleDirectory / "defines.json", Json::WriteStringArray(request.Defines));
}

void ShaderDebugArtifactWriter::WriteCompilerOutputs(
    const std::filesystem::path& bundleDirectory,
    const ShaderDebugArtifactSet& debugArtifacts,
    const CookedStageBuild& compiledStage)
{
	WriteText(bundleDirectory / "preprocessed-source.hlsl", debugArtifacts.PreprocessedSource);
	WriteText(bundleDirectory / "reflection.json", BuildReflectionJson(compiledStage.reflection));
	WriteText(bundleDirectory / "parameter-struct-match.json", debugArtifacts.ParameterMatchReportJson);
	if (!debugArtifacts.Disassembly.empty())
	{
		WriteText(bundleDirectory / "disassembly.txt", debugArtifacts.Disassembly);
	}
	WriteText(bundleDirectory / "compiler-stderr.txt", debugArtifacts.CompilerOutput);
	WriteText(bundleDirectory / "compile-args.json", Json::WriteStringArray(debugArtifacts.CompileArguments));
}

std::string ShaderDebugArtifactWriter::BuildBundleDirectoryName(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage)
{
	const std::string shaderId = Paths::MakeSafePathComponent(request.ShaderTypeName);
	return std::format(
	    "{}__{}__{}__{}",
	    shaderId,
	    Formatting::FormatHexUInt64(request.ShaderType),
	    Paths::MakeSafePathComponent(compiledStage.backendName),
	    Paths::MakeSafePathComponent(GetShaderTargetName(request.Target)));
}

std::string ShaderDebugArtifactWriter::BuildCompileRequestJson(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage)
{
	Json::ObjectWriter writer;
	writer.WriteHexUInt64("shaderTypeId", request.ShaderType);
	writer.WriteString("shaderType", request.ShaderTypeName);
	writer.WriteString("sourcePath", request.VirtualSourcePath);
	writer.WriteString("entryPoint", request.EntryPoint);
	writer.WriteString("stage", GetShaderStagePrefix(request.Stage));
	writer.WriteString("target", GetShaderTargetName(request.Target));
	writer.WriteString("backend", compiledStage.backendName);
	writer.WriteUInt64("backendVersion", compiledStage.backendVersion);
	writer.WriteString("format", compiledStage.format == ShaderBinaryFormat::SpirV ? "SpirV" : "Dxil");
	writer.WriteHexUInt64("sourceHash", compiledStage.sourceHash);
	writer.WriteHexUInt64("includeClosureHash", compiledStage.includeClosureHash);
	writer.WriteHexUInt64("requestHash", compiledStage.requestHash);
	writer.WriteHexUInt64("compileInputHash", compiledStage.compileInputHash);
	writer.WriteString("debugArtifact", compiledStage.debugArtifact);
	return writer.Finish();
}

std::string ShaderDebugArtifactWriter::BuildCompileIdentityJson(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage)
{
	Json::ObjectWriter writer;
	writer.WriteHexUInt64("compileInputHash", compiledStage.compileInputHash);
	writer.WriteHexUInt64("sourceHash", compiledStage.sourceHash);
	writer.WriteHexUInt64("includeClosureHash", compiledStage.includeClosureHash);
	writer.WriteHexUInt64("requestHash", compiledStage.requestHash);
	writer.WriteString("target", GetShaderTargetName(request.Target));
	writer.WriteString("format", compiledStage.format == ShaderBinaryFormat::SpirV ? "SpirV" : "Dxil");
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
		    "    {{ \"name\": {}, \"set\": {}, \"slot\": {}, \"arrayCount\": {}, \"sizeInBytes\": {}, "
		    "\"readOnly\": {}, \"constantBufferIndex\": {} }}",
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
