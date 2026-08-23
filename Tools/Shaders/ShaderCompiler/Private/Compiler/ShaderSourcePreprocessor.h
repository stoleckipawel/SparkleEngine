#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <string>
#include <string_view>

class ShaderSourcePreprocessor final
{
  public:
	static std::string Load(std::string_view sourcePath, const ShaderCompileOptions& options);

  private:
	struct PreprocessContext;

	static void VisitFile(
	    std::string_view filePath,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource);
	static std::string ReadSourceText(std::string_view virtualPath, const ShaderCompileOptions& options);
	static bool ContainsPragmaOnce(std::string_view sourceText);
	static std::string MakeLinePath(std::string_view path);
	static void AppendLineDirective(std::string& outSource, std::uint32_t lineNumber, std::string_view linePath);
	static void AppendExpandedInclude(
	    std::string_view includerPath,
	    std::string_view includeSpec,
	    const ShaderCompileOptions& options,
	    PreprocessContext& context,
	    std::string& outSource);
};
