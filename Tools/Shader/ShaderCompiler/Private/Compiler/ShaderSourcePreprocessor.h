#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

struct ShaderSourcePreprocessResult final
{
	std::string SourceText;
	std::string ErrorMessage;

	bool Succeeded() const noexcept { return ErrorMessage.empty(); }
};

class ShaderSourcePreprocessor final
{
  public:
	static ShaderSourcePreprocessResult Load(const std::filesystem::path& sourcePath, const ShaderCompileOptions& options);

  private:
	struct PreprocessContext;

	static bool VisitFile(
	    const std::filesystem::path& filePath,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource,
	    std::string& outErrorMessage);
	static bool TryReadSourceText(const std::filesystem::path& filePath, std::string& outSourceText, std::string& outErrorMessage);
	static bool ContainsPragmaOnce(std::string_view sourceText);
	static std::string MakeLinePath(const std::filesystem::path& path);
	static void AppendLineDirective(std::string& outSource, std::uint32_t lineNumber, std::string_view linePath);
	static bool AppendExpandedInclude(
	    const std::filesystem::path& includerPath,
	    std::string_view includeSpec,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource,
	    std::string& outErrorMessage);
};