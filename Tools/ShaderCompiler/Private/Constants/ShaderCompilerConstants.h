#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

inline constexpr int kExitCodeSuccess = 0;
inline constexpr int kExitCodeUsage = 1;
inline constexpr int kExitCodeManifestFailure = 5;
inline constexpr int kExitCodeCookFailure = 6;

inline constexpr std::size_t kSourceIdentityCanonicalReserveBytes = 256;
inline constexpr std::size_t kBinaryBlobInitialReserveBytes = 4096;

inline constexpr std::string_view kManifestHeaderSection = "ShaderCookManifest";
inline constexpr std::string_view kManifestPackageSectionPrefix = "Package ";
inline constexpr std::string_view kManifestKeyVersion = "Version";
inline constexpr std::string_view kManifestKeyBindingLayout = "BindingLayout";
inline constexpr std::string_view kManifestKeyVariant = "Variant";
inline constexpr std::string_view kManifestStageKeyPrefix = "Stage.";
inline constexpr std::string_view kManifestDefaultEntryPoint = "main";
inline constexpr char kManifestStageValueSeparator = '|';

inline constexpr std::string_view kRegistryHeaderSection = "[ShaderPackageRegistry]";
inline constexpr std::string_view kRegistryKeyVersion = "Version";
inline constexpr std::uint32_t kRegistryFormatVersion = 1;
inline constexpr std::string_view kRegistryKeyPackageCount = "PackageCount";
inline constexpr std::string_view kRegistryPackageSectionPrefix = "Package ";
inline constexpr std::string_view kRegistryKeyVariant = "Variant";
inline constexpr std::string_view kRegistryKeyBindingLayout = "BindingLayout";
inline constexpr std::string_view kRegistryKeyPackageKey = "PackageKey";
inline constexpr std::string_view kRegistryKeySourceIdentityHash = "SourceIdentityHash";
inline constexpr std::string_view kRegistryKeyBindingLayoutHash = "BindingLayoutHash";
inline constexpr std::string_view kRegistryKeyVariantHash = "VariantHash";
inline constexpr std::string_view kRegistryKeyDeclaredStages = "DeclaredStages";
inline constexpr std::string_view kRegistryKeyOutput = "Output";

inline constexpr std::string_view kCommandInspectManifest = "inspect-manifest";
inline constexpr std::string_view kCommandInspectManifestLegacy = "inspect-shader-manifest";
inline constexpr std::string_view kCommandCook = "cook";
inline constexpr std::string_view kCommandCookLegacy = "cook-shaders";

inline constexpr std::string_view kDxcContextLoggerCategory = "ShaderCompiler";
inline constexpr std::string_view kDxcCompilerLoggerCategory = "Tools.ShaderCompiler";
