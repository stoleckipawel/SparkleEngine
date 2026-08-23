#include "PCH.h"

#include "Compiler/ShaderSourcePreprocessor.h"

#include "Compiler/ShaderIncludeResolver.h"
#include "Compiler/ShaderSourceMountTable.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <regex>
#include <sstream>
#include <unordered_set>

struct ShaderSourcePreprocessor::PreprocessContext final
{
	std::unordered_set<std::string> OncePathKeys;
	std::unordered_set<std::string> ActivePathKeys;
};

void ShaderSourcePreprocessor::VisitFile(
    std::string_view filePath,
    const ShaderCompileRequest& request,
    PreprocessContext& context,
    std::string& outSource)
{
	const std::string pathKey = request.SourceMounts.get().CanonicalizeVirtualPath(filePath);

	const std::string sourceText = ReadSourceText(pathKey, request);

	if (ContainsPragmaOnce(sourceText) && !context.OncePathKeys.insert(pathKey).second)
	{
		return;
	}

	if (!context.ActivePathKeys.insert(pathKey).second)
	{
		throw Diagnostics::Error(std::format("Detected recursive shader include while preprocessing '{}'", pathKey));
	}

	const std::string linePath = MakeLinePath(pathKey);
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
			AppendExpandedInclude(pathKey, includeSpec, request, context, outSource);
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

std::string ShaderSourcePreprocessor::ReadSourceText(std::string_view virtualPath, const ShaderCompileRequest& request)
{
	const std::filesystem::path filePath = request.SourceMounts.get().ResolvePhysicalPath(virtualPath);
	std::vector<std::uint8_t> bytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(filePath, bytes, fileError))
	{
		throw Diagnostics::Error(std::format("Failed to read shader source '{}' while preprocessing includes.", virtualPath));
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

std::string ShaderSourcePreprocessor::MakeLinePath(std::string_view path)
{
	std::string linePath(path);
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

void ShaderSourcePreprocessor::AppendLineDirective(std::string& outSource, std::uint32_t lineNumber, std::string_view linePath)
{
	outSource += "#line ";
	outSource += std::to_string(lineNumber);
	outSource += " \"";
	outSource += linePath;
	outSource += "\"\n";
}

void ShaderSourcePreprocessor::AppendExpandedInclude(
    std::string_view includerPath,
    std::string_view includeSpec,
    const ShaderCompileRequest& request,
    PreprocessContext& context,
    std::string& outSource)
{
	const auto includePath = ShaderIncludeResolver::ResolveIncludePath(includerPath, includeSpec, request);
	if (!includePath)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Failed to resolve include '{}' referenced from '{}' while preprocessing shader source",
		        includeSpec,
		        includerPath));
	}

	VisitFile(*includePath, request, context, outSource);
}

std::string ShaderSourcePreprocessor::Load(std::string_view sourcePath, const ShaderCompileRequest& request)
{
	std::string sourceText;
	PreprocessContext context;
	VisitFile(sourcePath, request, context, sourceText);
	return sourceText;
}
