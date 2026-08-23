#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

inline constexpr int kExitCodeSuccess = 0;
inline constexpr int kExitCodeUsage = 1;
inline constexpr int kExitCodeNoWork = 2;
inline constexpr int kExitCodeCookFailure = 6;

inline constexpr std::size_t kSourceIdentityCanonicalReserveBytes = 256;
inline constexpr std::size_t kBinaryBlobInitialReserveBytes = 4096;

inline constexpr std::string_view kCommandCook = "cook";
inline constexpr std::string_view kCommandListBackends = "list-backends";
inline constexpr std::string_view kCommandListTargets = "list-targets";
inline constexpr std::string_view kCommandListShaders = "list-shaders";
inline constexpr std::string_view kCommandInspectShader = "inspect-shader";

inline constexpr std::string_view kDxcCompilerLoggerCategory = "Tools.ShaderCompiler";
