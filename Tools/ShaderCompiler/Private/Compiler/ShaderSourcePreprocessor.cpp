#include "PCH.h"

#include "Compiler/ShaderSourcePreprocessor.h"

#include "Compiler/ShaderCompilerPathUtils.h"
#include "Compiler/ShaderIncludeResolver.h"
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

bool ShaderSourcePreprocessor::VisitFile(
    const std::filesystem::path& filePath,
    const ShaderCompileOptions& options,
    PreprocessContext& context,
    std::string& outSource,
    std::string& outErrorMessage)
{
	const std::filesystem::path resolvedFilePath = ShaderCompilerPaths::CanonicalizeForCompiler(filePath);
	const std::wstring pathKey = ShaderIncludeResolver::MakeResolvedPathKey(resolvedFilePath);

	std::string sourceText;
	if (!TryReadSourceText(resolvedFilePath, sourceText, outErrorMessage))
	{
		return false;
	}

	if (ContainsPragmaOnce(sourceText) && !context.OncePathKeys.insert(pathKey).second)
	{
		return true;
	}

	if (!context.ActivePathKeys.insert(pathKey).second)
	{
		outErrorMessage = std::format("Detected recursive shader include while preprocessing '{}'", resolvedFilePath.string());
		return false;
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
			if (!AppendExpandedInclude(resolvedFilePath, includeSpec, options, context, outSource, outErrorMessage))
			{
				context.ActivePathKeys.erase(pathKey);
				return false;
			}
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
	return true;
}

bool ShaderSourcePreprocessor::TryReadSourceText(
    const std::filesystem::path& filePath,
    std::string& outSourceText,
    std::string& outErrorMessage)
{
	std::vector<std::uint8_t> bytes;
	if (!Files::TryReadAllBytes(filePath, bytes, outErrorMessage))
	{
		outErrorMessage = std::format(
		    "Failed to read shader source '{}' while preprocessing includes - {}",
		    filePath.string(),
		    outErrorMessage);
		return false;
	}

	outSourceText.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	return true;
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

bool ShaderSourcePreprocessor::AppendExpandedInclude(
    const std::filesystem::path& includerPath,
    std::string_view includeSpec,
    const ShaderCompileOptions& options,
    PreprocessContext& context,
    std::string& outSource,
    std::string& outErrorMessage)
{
	const auto includePath = ShaderIncludeResolver::ResolveIncludePath(includerPath, includeSpec, options);
	if (!includePath)
	{
		outErrorMessage = std::format(
		    "Failed to resolve include '{}' referenced from '{}' while preprocessing shader source",
		    includeSpec,
		    includerPath.string());
		return false;
	}

	return VisitFile(*includePath, options, context, outSource, outErrorMessage);
}

ShaderSourcePreprocessResult ShaderSourcePreprocessor::Load(
    const std::filesystem::path& sourcePath,
    const ShaderCompileOptions& options)
{
	ShaderSourcePreprocessResult result;
	PreprocessContext context;
	if (!VisitFile(sourcePath, options, context, result.SourceText, result.ErrorMessage))
	{
		result.SourceText.clear();
	}
	return result;
}
