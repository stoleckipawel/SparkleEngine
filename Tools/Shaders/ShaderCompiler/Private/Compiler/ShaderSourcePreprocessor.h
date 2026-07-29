#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

class ShaderSourcePreprocessor final
{
  public:
	static std::string Load(const std::filesystem::path& sourcePath, const ShaderCompileOptions& options);

  private:
	struct PreprocessContext;

	static void VisitFile(
	    const std::filesystem::path& filePath,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource);
	static std::string ReadSourceText(const std::filesystem::path& filePath);
	static bool ContainsPragmaOnce(std::string_view sourceText);
	static std::string MakeLinePath(const std::filesystem::path& path);
	static void AppendLineDirective(std::string& outSource, std::uint32_t lineNumber, std::string_view linePath);
	static void AppendExpandedInclude(
	    const std::filesystem::path& includerPath,
	    std::string_view includeSpec,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource);
};
