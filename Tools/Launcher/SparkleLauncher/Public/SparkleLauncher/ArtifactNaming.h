#pragma once

#include <string_view>

namespace SparkleLauncher::ArtifactNaming
{
	inline constexpr std::string_view kHostPrerequisite = "Host prerequisite";
	inline constexpr std::string_view kSourceDependencyGroup = "Source dependency group";
	inline constexpr std::string_view kRuntimeRedistributable = "Runtime redistributable";
	inline constexpr std::string_view kBuildOutput = "Build output";
	inline constexpr std::string_view kCookedOutput = "Cooked output";
	inline constexpr std::string_view kProjectSelection = "Project selection";

	inline constexpr std::string_view kVisibilityPublic = "public";
	inline constexpr std::string_view kVisibilityInternal = "internal";
	inline constexpr std::string_view kVisibilityPrivate = "private";

	inline constexpr std::string_view kBinaryTypeApp = "app";
	inline constexpr std::string_view kBinaryTypeDeveloperTool = "developer-tool";
	inline constexpr std::string_view kBinaryTypeRuntimeDll = "runtime-dll";
	inline constexpr std::string_view kBinaryTypePluginDll = "plugin-dll";
	inline constexpr std::string_view kBinaryTypeImportLibrary = "import-library";
	inline constexpr std::string_view kBinaryTypeStaticLibrary = "static-library";
	inline constexpr std::string_view kBinaryTypeSymbolFile = "symbol-file";
	inline constexpr std::string_view kBinaryTypeGeneratedAsset = "generated-asset";

	inline constexpr std::string_view kActionSyncSourceDependencies = "Prepare Workspace";
	inline constexpr std::string_view kActionGenerateProjectFiles = "Generate Build Files";
	inline constexpr std::string_view kActionOpenIde = "Open IDE";
}
