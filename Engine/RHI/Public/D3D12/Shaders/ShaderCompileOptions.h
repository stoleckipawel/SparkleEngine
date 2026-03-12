// ============================================================================
// ShaderCompileOptions.h
// ----------------------------------------------------------------------------
// Configuration structures for DXC shader compilation.
//
#pragma once

#include "RHIConfig.h"
#include <filesystem>
#include <vector>
#include <string>

// ============================================================================
// Shader Stage Enumeration
// ============================================================================

/// Identifies the programmable shader stage in the graphics pipeline.
enum class ShaderStage : uint8_t
{
	Vertex,    ///< Vertex shader - transforms vertices
	Pixel,     ///< Pixel shader - computes fragment colors
	Geometry,  ///< Geometry shader - processes primitives
	Hull,      ///< Hull shader - tessellation control
	Domain,    ///< Domain shader - tessellation evaluation
	Compute,   ///< Compute shader - general-purpose GPU compute
	Count      ///< Number of shader stages (for array sizing)
};

/// Returns the DXC target prefix for a shader stage (e.g., "vs" for Vertex).
inline const char* GetShaderStagePrefix(ShaderStage stage)
{
	static constexpr const char* kPrefixes[] = {"vs", "ps", "gs", "hs", "ds", "cs"};
	static_assert(std::size(kPrefixes) == static_cast<size_t>(ShaderStage::Count));
	return kPrefixes[static_cast<size_t>(stage)];
}

// ============================================================================
// Shader Compile Options
// ============================================================================

/// Configuration for a single shader compilation request.
struct ShaderCompileOptions
{
	std::filesystem::path SourcePath;        ///< Absolute path to the .hlsl file
	std::filesystem::path IncludeDir;        ///< Root directory for #include resolution
	std::string EntryPoint = "main";         ///< Entry function name
	ShaderStage Stage = ShaderStage::Pixel;  ///< Target shader stage

	// Feature flags
	bool EnableDebugInfo = false;       ///< Include debug symbols
	bool EnableOptimizations = true;    ///< Enable compiler optimizations
	bool TreatWarningsAsErrors = true;  ///< Promote warnings to errors
	bool StripReflection = true;        ///< Remove reflection data from output
	bool StripDebugInfo = true;         ///< Remove debug info from output

	/// Additional include directories beyond the primary IncludeDir.
	std::vector<std::filesystem::path> AdditionalIncludeDirs;

	/// Additional preprocessor defines (format: "NAME" or "NAME=VALUE").
	std::vector<std::string> Defines;

	/// Builds the shader model target string (e.g., "vs_6_0").
	std::string BuildTargetProfile() const
	{
		std::string profile;
		profile.reserve(8);
		profile += GetShaderStagePrefix(Stage);
		profile += '_';
		profile += std::to_string(RHISettings::ShaderModelMajor);
		profile += '_';
		profile += std::to_string(RHISettings::ShaderModelMinor);
		return profile;
	}
};
