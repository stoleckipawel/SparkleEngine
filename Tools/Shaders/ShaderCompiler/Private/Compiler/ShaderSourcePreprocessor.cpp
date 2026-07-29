#include "PCH.h"

#include "Compiler/ShaderSourcePreprocessor.h"

#include "Compiler/ShaderCompilerPaths.h"
#include "Compiler/ShaderIncludeResolver.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <regex>
#include <sstream>
#include <unordered_set>

struct ShaderSourcePreprocessor::PreprocessContext final
{
	std::unordered_set<std::wstring> OncePathKeys;
	std::unordered_set<std::wstring> ActivePathKeys;
};

void ShaderSourcePreprocessor::VisitFile(
    const std::filesystem::path& filePath,
    const ShaderCompileOptions& options,
    PreprocessContext& context,
    std::string& outSource)
{
	const std::filesystem::path resolvedFilePath = ShaderCompilerPaths::CanonicalizeForCompiler(filePath);
	const std::wstring pathKey = ShaderIncludeResolver::MakeResolvedPathKey(resolvedFilePath);

	const std::string sourceText = ReadSourceText(resolvedFilePath);

	if (ContainsPragmaOnce(sourceText) && !context.OncePathKeys.insert(pathKey).second)
	{
		return;
	}

	if (!context.ActivePathKeys.insert(pathKey).second)
	{
		throw Diagnostics::Error(
		    std::format("Detected recursive shader include while preprocessing '{}'", resolvedFilePath.string()));
	}

	const std::string linePath = MakeLinePath(resolvedFilePath);
	AppendLineDirective(outSource, 1u, linePath);

	static const std::regex includeRegex(R"(^\s*#\s*include\s*["<]([^">]+)[">])", std::regex::icase);
	static const std::regex pragmaOnceRegex(R"(^\s*#\s*pragma\s+once\b)", std::regex::icase);

	std::istringstream stream(sourceText);
	std::string line;
	std::smatch match;
	std::uint32_t lineNumber = 1;
	while (std::getline(stream, line))
	{
		if (std::regex_search(line, pragmaOnceRegex))
		{
			++lineNumber;
			continue;
		}

		if (std::regex_search(line, match, includeRegex))
		{
			const std::string includeSpec = match[1].str();
			AppendExpandedInclude(resolvedFilePath, includeSpec, options, context, outSource);
			AppendLineDirective(outSource, lineNumber + 1u, linePath);
		}
		else
		{
			outSource += line;
			outSource += '\n';
		}

		++lineNumber;
	}

	context.ActivePathKeys.erase(pathKey);
}

std::string ShaderSourcePreprocessor::ReadSourceText(const std::filesystem::path& filePath)
{
	std::vector<std::uint8_t> bytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(filePath, bytes, fileError))
	{
		throw Diagnostics::Error(std::format(
		    "Failed to read shader source '{}' while preprocessing includes - {}",
		    filePath.string(),
		    fileError));
	}

	return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

bool ShaderSourcePreprocessor::ContainsPragmaOnce(std::string_view sourceText)
{
	static const std::regex pragmaOnceRegex(R"(^\s*#\s*pragma\s+once\b)", std::regex::icase);
	std::istringstream stream(std::string{sourceText});
	std::string line;
	while (std::getline(stream, line))
	{
		if (std::regex_search(line, pragmaOnceRegex))
		{
			return true;
		}
	}
	return false;
}

std::string ShaderSourcePreprocessor::MakeLinePath(const std::filesystem::path& path)
{
	std::string linePath = ShaderCompilerPaths::MakePathArgument(path);
	for (std::size_t index = 0; index < linePath.size(); ++index)
	{
		if (linePath[index] == '"')
		{
			linePath.insert(index, 1u, '\\');
			++index;
		}
	}
	return linePath;
}

void ShaderSourcePreprocessor::AppendLineDirective(
    std::string& outSource,
    std::uint32_t lineNumber,
    std::string_view linePath)
{
	outSource += "#line ";
	outSource += std::to_string(lineNumber);
	outSource += " \"";
	outSource += linePath;
	outSource += "\"\n";
}

void ShaderSourcePreprocessor::AppendExpandedInclude(
    const std::filesystem::path& includerPath,
    std::string_view includeSpec,
    const ShaderCompileOptions& options,
    PreprocessContext& context,
    std::string& outSource)
{
	const auto includePath = ShaderIncludeResolver::ResolveIncludePath(includerPath, includeSpec, options);
	if (!includePath)
	{
		throw Diagnostics::Error(std::format(
		    "Failed to resolve include '{}' referenced from '{}' while preprocessing shader source",
		    includeSpec,
		    includerPath.string()));
	}

	VisitFile(*includePath, options, context, outSource);
}

std::string ShaderSourcePreprocessor::Load(
    const std::filesystem::path& sourcePath,
    const ShaderCompileOptions& options)
{
	std::string sourceText;
	PreprocessContext context;
	VisitFile(sourcePath, options, context, sourceText);
	return sourceText;
}
