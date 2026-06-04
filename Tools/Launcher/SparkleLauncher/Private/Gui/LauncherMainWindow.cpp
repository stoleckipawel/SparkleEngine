#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherBackend.h"
#include "LauncherProjectModel.h"
#include "LauncherOutputWidgets.h"
#include "LauncherSettings.h"
#include "LauncherVisualStyle.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtCore/QDateTime>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QDesktopServices>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include <QtCore/QUrl>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <system_error>
#include <utility>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace SparkleLauncher
{
	static constexpr int kMaxOperationOutputCharacters = 1000000;
	static constexpr int kSpaceTiny = 2;
	static constexpr int kSpaceSmall = 8;
	static constexpr int kSpaceMedium = 12;
	static constexpr int kSpaceLarge = 16;
	static constexpr int kPanelHorizontalMargin = 18;
	static constexpr int kPanelVerticalMargin = 14;
	static constexpr int kWorkflowRailWidth = 80;
	static constexpr int kWorkflowGroupMinHeight = 54;
	static constexpr int kWorkflowButtonMinHeight = 32;
	static constexpr int kFieldLabelWidth = 116;
	static constexpr int kOperationOutputMinHeight = 96;
	static constexpr int kOperationOutputCompactMaxHeight = 128;
	static constexpr int kOperationOutputProminentMinHeight = 136;
	static constexpr int kOperationOutputMaxHeight = 220;
	static constexpr int kLauncherIconSize = 14;
	static constexpr int kStatusChipColumnWidth = 118;
	static constexpr int kStatusActionColumnWidth = 28;
	static constexpr const char* kColorStateQueued = "#8b949e";
	static constexpr const char* kColorStateRunning = "#76b900";
	static constexpr const char* kColorStateSuccess = "#7ee787";
	static constexpr const char* kColorStateDestructive = "#ff7b72";
	static constexpr const char* kColorStateWarning = "#ffb454";
	struct CleanScopeUiOption
	{
		QString Label;
		QString Value;
		QString Detail;
		QString Preview;
		QString Group;
	};

	struct ThirdPartyDependencyUiEntry
	{
		QString Label;
		QString Version;
		QString Purpose;
		QString CacheDirectoryName;
	};

	struct DependencyGroupUiEntry
	{
		QString Id;
		QString Label;
		QString Summary;
		QString UnlockSummary;
		QString ConfigureOption;
		bool Required = false;
		bool Enabled = false;
		std::vector<ThirdPartyDependencyUiEntry> Dependencies;
	};

	struct HomeNextAction
	{
		QString OperationId;
		QString Label;
		QString Detail;
		bool NavigateOnly = false;
	};

	static void ApplyNativeDarkTitleBar(QWidget& window)
	{
#ifdef Q_OS_WIN
		using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
		HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
		if (dwmapi == nullptr)
		{
			return;
		}

		auto setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
		if (setWindowAttribute == nullptr)
		{
			FreeLibrary(dwmapi);
			return;
		}

		HWND hwnd = reinterpret_cast<HWND>(window.winId());
		BOOL darkMode = TRUE;
		constexpr DWORD kDwmUseImmersiveDarkMode = 20;
		constexpr DWORD kDwmUseImmersiveDarkModeLegacy = 19;
		HRESULT result = setWindowAttribute(hwnd, kDwmUseImmersiveDarkMode, &darkMode, sizeof(darkMode));
		if (FAILED(result))
		{
			setWindowAttribute(hwnd, kDwmUseImmersiveDarkModeLegacy, &darkMode, sizeof(darkMode));
		}

		constexpr DWORD kDwmCaptionColor = 35;
		constexpr DWORD kDwmTextColor = 36;
		const COLORREF captionColor = RGB(17, 19, 18);
		const COLORREF textColor = RGB(242, 244, 241);
		setWindowAttribute(hwnd, kDwmCaptionColor, &captionColor, sizeof(captionColor));
		setWindowAttribute(hwnd, kDwmTextColor, &textColor, sizeof(textColor));
		FreeLibrary(dwmapi);
#else
		Q_UNUSED(window);
#endif
	}

	static const std::vector<DependencyGroupUiEntry>& GetDependencyGroups()
	{
		static const std::vector<DependencyGroupUiEntry> groups = [] {
			std::vector<DependencyGroupUiEntry> entries;
			entries.push_back({
			    "core-workspace",
			    "Core Workspace Source Tier",
			    "Baseline shared dependencies used by the launcher, engine, and project builds.",
			    "Required source tier for local rebuilds. Runtime packages can still launch bundled components without rebuilding this tier.",
			    QString(),
			    true,
			    true,
			    {
			        {"Dear ImGui", "v1.92.5", "Immediate-mode UI core and Win32 platform backend.", "imgui-src"},
			        {"spdlog", "v1.14.1", "Repo-wide logging backend.", "spdlog-src"},
			        {"Font Awesome Free Solid", "v6.7.1", "Launcher/editor icon font asset and license.", "editor-icons"},
			    }});
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			const bool contentPipelineEnabled = true;
#else
			const bool contentPipelineEnabled = false;
#endif
			entries.push_back({
			    "content-pipeline",
			    "Content Pipeline Source Tier",
			    "Optional source import, mesh cook, and texture cook dependencies.",
			    "Unlocks Build Cooking Tools, Cook Textures, Cook Scenes And Meshes, and the content phase of Cook All.",
			    "SPARKLE_ENABLE_CONTENT_PIPELINE",
			    false,
			    contentPipelineEnabled,
			    {
			        {"cgltf", "v1.15", "Single-header glTF 2.0 parser for source scene imports.", "cgltf-src"},
			        {"stb", "master", "Header-only image loading and mip resize helpers.", "stb-src"},
			        {"tinyexr", "v1.0.7", "Header-only OpenEXR image loading support.", "tinyexr-src"},
			        {"zlib", "v1.3.1", "Compression backend used by Assimp.", "zlib-src"},
			        {"Assimp", "v5.4.3", "FBX and DCC scene import support.", "assimp-src"},
			        {"Compressonator", "master (sparse)", "AMD BC1-BC7 texture block compression support.", "compressonator-src"},
			    }});
#if SPARKLE_ENABLE_KTX_SUPPORT
			const bool ktxSupportEnabled = true;
#else
			const bool ktxSupportEnabled = false;
#endif
			entries.push_back({
			    "ktx-support",
			    "KTX Container Source Tier",
			    "Optional KTX2 container support layered on top of the texture pipeline.",
			    "Extends texture workflows when the repo is configured for KTX support.",
			    "SPARKLE_ENABLE_KTX_SUPPORT",
			    false,
			    ktxSupportEnabled,
			    {
			        {"KTX-Software", "v4.3.2", "KTX2 texture container read/write support.", "ktx-src"},
			    }});
#if SPARKLE_ENABLE_SHADER_COMPILER
			const bool shaderCompilerEnabled = true;
#else
			const bool shaderCompilerEnabled = false;
#endif
			entries.push_back({
			    "shader-compiler",
			    "Shader Compiler Source Tier",
			    "Optional offline shader compiler dependencies.",
			    "Unlocks Build Cooking Tools, Cook Shaders, and the shader phase of Cook All.",
			    "SPARKLE_ENABLE_SHADER_COMPILER",
			    false,
			    shaderCompilerEnabled,
			    {
			        {"SPIRV-Reflect", "vulkan-sdk-1.3.290.0", "SPIR-V reflection for offline shader compiler backends.", "spirv_reflect-src"},
			    }});
			return entries;
		}();
		return groups;
	}

	static const std::vector<ThirdPartyDependencyUiEntry>& GetTrackedThirdPartyDependencies()
	{
		static const std::vector<ThirdPartyDependencyUiEntry> dependencies = [] {
			std::vector<ThirdPartyDependencyUiEntry> entries;
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				entries.insert(entries.end(), group.Dependencies.begin(), group.Dependencies.end());
			}
			return entries;
		}();
		return dependencies;
	}

	static QString ToDisplayPath(const std::filesystem::path& repositoryRoot, const std::filesystem::path& path)
	{
		std::error_code errorCode;
		const std::filesystem::path relative = std::filesystem::relative(path, repositoryRoot, errorCode);
		return QString::fromStdString((!errorCode && !relative.empty()) ? relative.generic_string() : path.generic_string());
	}

	static QString FormatDirectoryInventory(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode) || errorCode)
		{
			return "not present";
		}

		if (std::filesystem::is_regular_file(path, errorCode))
		{
			return "1 file";
		}

		std::uintmax_t fileCount = 0;
		std::uintmax_t directoryCount = 0;
		if (std::filesystem::is_directory(path, errorCode))
		{
			std::filesystem::recursive_directory_iterator iterator(
			    path,
			    std::filesystem::directory_options::skip_permission_denied,
			    errorCode);
			const std::filesystem::recursive_directory_iterator end;
			while (iterator != end)
			{
				const std::filesystem::directory_entry entry = *iterator;
				if (entry.is_directory(errorCode))
				{
					++directoryCount;
				}
				else if (entry.is_regular_file(errorCode))
				{
					++fileCount;
				}
				errorCode.clear();
				iterator.increment(errorCode);
				errorCode.clear();
			}
		}

		return QStringLiteral("%1 files, %2 folders").arg(fileCount).arg(directoryCount);
	}

	static std::filesystem::path ResolveCleanScopePreviewPath(const std::filesystem::path& repositoryRoot, const QString& projectId, const QString& scope)
	{
		if (scope == "selected-cooked")
		{
			return GetCookedProjectDirectory(repositoryRoot, projectId.toStdString());
		}
		if (scope == "all-cooked")
		{
			return GetCookedProjectsArtifactDirectory(repositoryRoot);
		}
		if (scope == "build-tree")
		{
			return GetBuildDirectory(repositoryRoot);
		}
		if (scope == "shader-cache")
		{
			return GetBuildDirectory(repositoryRoot) / "Cache" / "Shaders";
		}
		if (scope == "deps")
		{
			return GetBuildDirectory(repositoryRoot) / "_deps";
		}
		if (scope == "logs")
		{
			return repositoryRoot / "logs";
		}
		return repositoryRoot;
	}

	static QString CleanScopeDisplayName(const QString& scopeValue)
	{
		if (scopeValue == "selected-cooked")
		{
			return "Project Cooked Outputs";
		}
		if (scopeValue == "all-cooked")
		{
			return "All Cooked Outputs";
		}
		if (scopeValue == "build-tree")
		{
			return "Build Outputs";
		}
		if (scopeValue == "shader-cache")
		{
			return "Shader Cache";
		}
		if (scopeValue == "deps")
		{
			return "Source Dependency Cache";
		}
		if (scopeValue == "logs")
		{
			return "Log Files";
		}
		if (scopeValue == "pristine")
		{
			return "Generated Workspace";
		}
		return scopeValue;
	}

	static QString FormatStatusPath(const std::filesystem::path& path)
	{
		return path.empty() ? QString() : QString::fromStdString(path.string());
	}

	static bool DirectoryHasEntries(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(path, errorCode))
		{
			return false;
		}
		return std::filesystem::directory_iterator(path, errorCode) != std::filesystem::directory_iterator();
	}

	static bool PathExists(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::exists(path, errorCode) && !errorCode;
	}

	static bool ReadinessContains(const std::vector<std::string>& messages, const QString& needle)
	{
		for (const std::string& message : messages)
		{
			if (QString::fromStdString(message).contains(needle, Qt::CaseInsensitive))
			{
				return true;
			}
		}
		return false;
	}

	static QString FirstReadinessContaining(const std::vector<std::string>& messages, const QString& needle)
	{
		for (const std::string& message : messages)
		{
			const QString text = QString::fromStdString(message);
			if (text.contains(needle, Qt::CaseInsensitive))
			{
				return text;
			}
		}
		return QString();
	}

	static QString FormatTrackedDependencySummary(const std::filesystem::path& dependencyCachePath)
	{
		int readyCount = 0;
		int trackedCount = 0;
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			if (!group.Enabled)
			{
				continue;
			}
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				++trackedCount;
				if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
				{
					++readyCount;
				}
			}
		}

		return QStringLiteral("%1 of %2 enabled tracked dependencies are cached.")
		    .arg(readyCount)
		    .arg(trackedCount);
	}

	static int CountReadyDependencies(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		int readyCount = 0;
		for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
		{
			if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
			{
				++readyCount;
			}
		}
		return readyCount;
	}

	static QString DependencyGroupStatusText(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "Disabled";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return group.Required ? "Ready" : "Cached";
		}
		if (readyCount > 0)
		{
			return "Partial";
		}
		return group.Required ? "Pending sync" : "Available";
	}

	static QString DependencyGroupStatusState(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "neutral";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return "ok";
		}
		return "warning";
	}

	static QString FormatDependencyGroupDetail(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath, int readyCount)
	{
		Q_UNUSED(dependencyCachePath);
		QString detail = group.Summary + " " + group.UnlockSummary;
		if (!group.Enabled)
		{
			return detail + QStringLiteral(" Disabled by %1=OFF in this workspace configuration.").arg(group.ConfigureOption);
		}
		return detail + QStringLiteral(" %1 of %2 tracked dependencies are cached.")
		                    .arg(readyCount)
		                    .arg(group.Dependencies.size());
	}

	static QString FormatDependencyEntryDetail(const DependencyGroupUiEntry& group, const ThirdPartyDependencyUiEntry& dependency, const std::filesystem::path& dependencyPath)
	{
		QString detail = QStringLiteral("%1 Cache directory: %2 under the source dependency cache.")
		                     .arg(dependency.Purpose)
		                     .arg(QString::fromStdString(dependencyPath.filename().generic_string()));
		if (!group.Enabled)
		{
			detail += QStringLiteral(" Disabled by %1=OFF in this workspace configuration.").arg(group.ConfigureOption);
		}
		return detail;
	}

	static bool OperationUsesDependencyGroup(const QString& operationId, const DependencyGroupUiEntry& group)
	{
		if (operationId == "workspace.setup")
		{
			return true;
		}
		if (group.Id == "core-workspace")
		{
			return operationId == "workspace.generate-solution" || operationId == "workspace.open-solution" || operationId == "workspace.build-all" ||
			    operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId.startsWith("cook.");
		}
		if (group.Id == "content-pipeline")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.textures" ||
			    operationId == "cook.assets" || operationId == "cook.project";
		}
		if (group.Id == "shader-compiler")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.shaders" ||
			    operationId == "cook.project";
		}
		if (group.Id == "ktx-support")
		{
			return operationId == "workspace.setup" || operationId == "cook.textures" || operationId == "cook.project";
		}
		return false;
	}

	static QString ToolchainStatusState(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "ok";
		case ToolchainItemState::Warning:
			return required ? "warning" : "neutral";
		case ToolchainItemState::Missing:
			return required ? "bad" : "neutral";
		}
		return "neutral";
	}

	static QString ToolchainStatusText(ToolchainItemState state, bool required)
	{
		switch (state)
		{
		case ToolchainItemState::Found:
			return "Ready";
		case ToolchainItemState::Warning:
			return required ? "Warning" : "Optional";
		case ToolchainItemState::Missing:
			return required ? "Missing" : "Optional";
		}
		return "Unknown";
	}

	static QString BuildGeneratorSummary(const BuildToolchainStatus& toolchain)
	{
		return QStringLiteral("Generator: %1 | Platform: %2%3%4")
		    .arg(QString::fromStdString(toolchain.Generator))
		    .arg(QString::fromStdString(toolchain.Platform))
		    .arg(toolchain.Toolset.empty() ? QString() : QStringLiteral(" | Toolset: %1").arg(QString::fromStdString(toolchain.Toolset)))
		    .arg(toolchain.QtRootPath.empty() ? QString() : QStringLiteral(" | Qt: %1").arg(QString::fromStdString(toolchain.QtRootPath.string())));
	}

	static QString RequiredToolProblemSummary(const BuildToolchainStatus& toolchain)
	{
		QStringList problems;
		for (const ToolchainItemStatus& item : toolchain.Items)
		{
			if (!item.Required || item.State == ToolchainItemState::Found)
			{
				continue;
			}
			problems.push_back(QString::fromStdString(item.DisplayName));
		}

		return problems.isEmpty() ? QString() : "Missing or blocked: " + problems.join(", ");
	}

	static QString CombineStatusDetail(const QString& first, const QString& second)
	{
		if (first.isEmpty())
		{
			return second;
		}
		if (second.isEmpty())
		{
			return first;
		}
		return first + " | " + second;
	}

	static QString BuildFilesRecoveryHint(const BuildFilesFreshnessStatus& freshness)
	{
		switch (freshness.State)
		{
		case BuildFilesFreshnessState::GeneratorMismatch:
			return "Recovery: clean Build Outputs or choose a different build directory before running Generate Build Files again.";
		case BuildFilesFreshnessState::BuildDirectoryMissing:
		case BuildFilesFreshnessState::CMakeCacheMissing:
		case BuildFilesFreshnessState::SolutionMissing:
		case BuildFilesFreshnessState::FreshnessStampMissing:
		case BuildFilesFreshnessState::FreshnessStampMismatch:
		case BuildFilesFreshnessState::SourceListChanged:
		case BuildFilesFreshnessState::BuildInputChanged:
			return "Recovery: run Generate Build Files to refresh generated CMake and IDE state.";
		case BuildFilesFreshnessState::Current:
		case BuildFilesFreshnessState::Unsupported:
			return QString();
		}
		return QString();
	}

	static HomeNextAction RecoveryActionForFailure(const QString& operationId, const QString& statusText)
	{
		if (statusText.contains("generator platform", Qt::CaseInsensitive) || statusText.contains("CMakeCache", Qt::CaseInsensitive) ||
		    statusText.contains("Generate Workspace", Qt::CaseInsensitive) || statusText.contains("Generated workspace", Qt::CaseInsensitive))
		{
			return {"workspace.generate-solution", "Generate Build Files", "Refresh generated CMake and IDE files for the selected toolchain.", true};
		}
		if (statusText.contains("tool", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("Visual Studio", Qt::CaseInsensitive) ||
		    statusText.contains("Qt", Qt::CaseInsensitive))
		{
			return {"toolchain.check", "Verify Host Environment", "Audit installed host prerequisites and selected toolchain.", true};
		}
		if (operationId == "project.open.editor" || (operationId == "project.run" && statusText.contains("Editor", Qt::CaseInsensitive)))
		{
			return {"project.build.editor", "Build Editor", "Rebuild the selected project editor artifact.", true};
		}
		if (operationId == "project.open.runtime" || statusText.contains("Runtime", Qt::CaseInsensitive))
		{
			return {"project.build.runtime", "Build Runtime", "Rebuild the selected project runtime artifact.", true};
		}
		if (statusText.contains("scene", Qt::CaseInsensitive) || statusText.contains("mesh", Qt::CaseInsensitive))
		{
			return {"cook.assets", "Cook Scenes And Meshes", "Refresh scene and mesh cooked outputs.", true};
		}
		if (statusText.contains("texture", Qt::CaseInsensitive))
		{
			return {"cook.textures", "Cook Textures", "Refresh texture cooked outputs.", true};
		}
		if (statusText.contains("shader", Qt::CaseInsensitive))
		{
			return {"cook.shaders", "Cook Shaders", "Refresh shader cooked outputs.", true};
		}
		if (operationId.startsWith("cook."))
		{
			return {"cook.tools.prepare", "Build Cooking Tools", "Build local cook tool outputs before retrying cook workflows.", true};
		}
		if (operationId.startsWith("project.build") || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId == "cook.tools.prepare")
		{
			return {"toolchain.check", "Verify Host Environment", "Audit installed host prerequisites before retrying the build.", true};
		}
		return {};
	}

	static QString SanitizeActionHistoryField(QString value)
	{
		value.replace('\t', ' ');
		value.replace('\r', ' ');
		value.replace('\n', ' ');
		return value.trimmed();
	}

	static WorkspaceIde SelectedWorkspaceIde(const LauncherSettings& settings)
	{
		WorkspaceIde ide = WorkspaceIde::VisualStudio;
		TryParseWorkspaceIde(settings.WorkspaceIde().toStdString(), ide);
		return ide;
	}

	static QString SelectedWorkspaceIdeName(const LauncherSettings& settings)
	{
		return QString::fromStdString(DisplayName(SelectedWorkspaceIde(settings)));
	}

	static QString ResolveShaderTargetSelection(const LauncherSettings& settings)
	{
		const QString preset = settings.ShaderTargetPreset();
		if (preset == "d3d12")
		{
			return "DxilSm66";
		}
		if (preset == "vulkan")
		{
			return "SpirV16";
		}
		if (preset == "dxil-all")
		{
			return "DxilSm60, DxilSm61, DxilSm62, DxilSm63, DxilSm64, DxilSm65, DxilSm66, DxilSm67";
		}
		if (preset == "spirv-all")
		{
			return "SpirV14, SpirV15, SpirV16";
		}
		if (preset == "custom")
		{
			return settings.ShaderCustomTargets();
		}
		return "DxilSm66, SpirV16";
	}

	static QString ResolvedShaderDebugArtifactDirectory(const std::filesystem::path& repositoryRoot, const LauncherProjectModel& projectModel, const LauncherSettings& settings)
	{
		if (!settings.ShaderDebugArtifactDirectory().trimmed().isEmpty())
		{
			return settings.ShaderDebugArtifactDirectory().trimmed();
		}

		const QString projectId = projectModel.SelectedProjectId().isEmpty() ? "Workspace" : projectModel.SelectedProjectId();
		return QString::fromStdString((GetDiagnosticsDirectory(repositoryRoot) / "ShaderDebugArtifacts" / projectId.toStdString()).string());
	}

	static BuildWorkspaceOperationRequest MakeWorkspacePlanRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherProjectModel& projectModel,
	    const LauncherSettings& settings)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = repositoryRoot;
		request.ProjectId = projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : projectModel.SelectedProjectId().toStdString();
		request.EditorProfile = settings.EditorProfile().toStdString();
		request.RuntimeProfile = settings.RuntimeProfile().toStdString();
		request.PreferredIde = SelectedWorkspaceIde(settings);
		request.ForceConfigure = settings.ForceConfigure();
		return request;
	}

	static QString FirstBlockingReadinessMessage(const BuildWorkspaceOperationPlan& plan)
	{
		for (const std::string& message : plan.ReadinessMessages)
		{
			if (!message.empty())
			{
				return QString::fromStdString(message);
			}
		}

		return "This workflow is currently blocked.";
	}

	static QString FirstBlockingReadinessMessage(const std::vector<std::string>& readinessMessages)
	{
		for (const std::string& message : readinessMessages)
		{
			if (!message.empty())
			{
				return QString::fromStdString(message);
			}
		}

		return "This workflow is currently blocked.";
	}

	static void AddExplicitCleanTarget(
	    QVector<LauncherCleanTarget>& targets,
	    const QString& displayName,
	    const std::filesystem::path& path,
	    const QString& detail)
	{
		LauncherCleanTarget target;
		target.DisplayName = displayName;
		target.Path = QString::fromStdString(path.string());
		target.Detail = detail;
		targets.push_back(std::move(target));
	}

	static void AddTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& targetName,
	    const QString& detail,
	    const std::filesystem::path& preservedPath = {})
	{
		std::filesystem::path binaryDirectory = GetDeveloperArtifactDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		std::filesystem::path libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "runtime-support/" + targetName.toStdString(), profileName.toStdString());
		std::filesystem::path symbolDirectory = GetSymbolDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		if (targetName == "SparkleLauncher" || targetName == "SparkleLauncherProbe")
		{
			binaryDirectory = GetLauncherArtifactDirectory(repositoryRoot, profileName.toStdString());
			libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "launcher", profileName.toStdString());
			symbolDirectory = GetSymbolDirectory(repositoryRoot) / "launcher" / profileName.toStdString();
		}
		else if (targetName == "AssetCooker" || targetName == "TextureCooker" || targetName == "ShaderCompiler" || targetName == "AssetConverter")
		{
			binaryDirectory = GetDevelopmentToolArtifactDirectory(repositoryRoot, targetName.toStdString(), profileName.toStdString());
			libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "tools/" + targetName.toStdString(), profileName.toStdString());
			symbolDirectory = GetSymbolDirectory(repositoryRoot) / "tools" / targetName.toStdString() / profileName.toStdString();
		}
		const std::filesystem::path executablePath = binaryDirectory / (targetName.toStdString() + ".exe");
		if (preservedPath.empty() || executablePath != preservedPath)
		{
			AddExplicitCleanTarget(targets, targetName + " executable", executablePath, detail);
		}
		AddExplicitCleanTarget(targets, targetName + " program database", symbolDirectory / (targetName.toStdString() + ".pdb"), detail);
		AddExplicitCleanTarget(targets, targetName + " import library", libraryDirectory / (targetName.toStdString() + ".lib"), detail);
		AddExplicitCleanTarget(targets, targetName + " compile database", symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"), detail);
	}

	static void AddProjectTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& projectName,
	    const QString& productRole,
	    const QString& targetName,
	    const QString& detail)
	{
		const std::filesystem::path binaryDirectory = GetProjectTargetArtifactDirectory(repositoryRoot, projectName.toStdString(), productRole.toStdString(), profileName.toStdString());
		const std::filesystem::path libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "projects/" + projectName.toStdString() + "/" + productRole.toStdString(), profileName.toStdString());
		const std::filesystem::path symbolDirectory = GetSymbolDirectory(repositoryRoot) / "projects" / projectName.toStdString() / productRole.toStdString() / profileName.toStdString();
		const std::filesystem::path executablePath = binaryDirectory / (targetName.toStdString() + ".exe");
		AddExplicitCleanTarget(targets, targetName + " executable", executablePath, detail);
		AddExplicitCleanTarget(targets, targetName + " program database", symbolDirectory / (targetName.toStdString() + ".pdb"), detail);
		AddExplicitCleanTarget(targets, targetName + " import library", libraryDirectory / (targetName.toStdString() + ".lib"), detail);
		AddExplicitCleanTarget(targets, targetName + " compile database", symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"), detail);
	}

	LauncherMainWindow::LauncherMainWindow(
	    std::filesystem::path repositoryRoot,
	    LauncherProjectModel& projectModel,
	    LauncherSettings& settings,
	    LauncherBackend& backend,
	    QWidget* parent)
	    : QMainWindow(parent)
	    , m_repositoryRoot(std::move(repositoryRoot))
	    , m_projectModel(projectModel)
	    , m_settings(settings)
	    , m_backend(backend)
	{
		LoadActionHistory();
		setWindowTitle("Sparkle Launcher");
		setMinimumSize(980, 620);
		resize(1240, 800);
		LoadLauncherIconFont();
		const QIcon applicationIcon = CreateApplicationIcon();
		QGuiApplication::setWindowIcon(applicationIcon);
		setWindowIcon(applicationIcon);

		QWidget* centralWidget = new QWidget(this);
		QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
		rootLayout->setContentsMargins(0, 0, 0, 0);
		rootLayout->setSpacing(0);

		QHBoxLayout* bodyLayout = new QHBoxLayout();
		bodyLayout->setContentsMargins(0, 0, 0, 0);
		bodyLayout->setSpacing(0);
		bodyLayout->addWidget(CreateWorkflowSurface(), 1);
		rootLayout->addLayout(bodyLayout, 1);
		setCentralWidget(centralWidget);
		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		if (!workflows.empty() && !workflows.front().OperationIds.empty())
		{
			SetSelectedOperation(workflows.front().OperationIds.front());
		}

		ConfigureTabOrder();
		ApplyVisualStyle();
		ApplyNativeDarkTitleBar(*this);

		connect(&m_projectModel, &LauncherProjectModel::ProjectsChanged, this, &LauncherMainWindow::PopulateProjectSelectors);
		connect(&m_projectModel, &LauncherProjectModel::SelectionChanged, this, [this](const QString&) {
			PopulateProjectSelectors();
			RebuildOptionsPages();
			UpdateRunAvailability();
		});
		connect(&m_projectModel, &LauncherProjectModel::ProjectDiscoveryFailed, this, &LauncherMainWindow::SetStartupNotice);
		connect(&m_settings, &LauncherSettings::SettingsChanged, this, [this]() {
			RebuildOptionsPages();
			UpdateRunAvailability();
		});
		connect(&m_backend, &LauncherBackend::OperationStarted, this, &LauncherMainWindow::DisplayOperationStarted);
		connect(&m_backend, &LauncherBackend::OperationOutputReceived, this, &LauncherMainWindow::AppendOperationOutput);
		connect(&m_backend, &LauncherBackend::OperationFinished, this, &LauncherMainWindow::DisplayOperationFinished);

		UpdateProgress();
		QTimer::singleShot(0, this, &LauncherMainWindow::RefreshProjects);
	}

	void LauncherMainWindow::SetStartupNotice(const QString& message)
	{
		if (!message.isEmpty())
		{
			SetStatusMessage("Project discovery: " + message);
			UpdateRunAvailability();
		}
	}

	void LauncherMainWindow::RefreshProjects()
	{
		m_projectModel.Refresh(m_repositoryRoot);
	}

	void LauncherMainWindow::SelectWorkflowGroupButton(QAbstractButton* button)
	{
		if (button == nullptr || m_operationStack == nullptr)
		{
			return;
		}

		const int workflowIndex = button->property("WorkflowIndex").toInt();
		if (workflowIndex >= 0 && workflowIndex < m_operationStack->count())
		{
			m_operationStack->setCurrentIndex(workflowIndex);
			SetActiveWorkflowGroup(workflowIndex);

			const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
			if (workflowIndex < workflows.size() && !workflows[workflowIndex].OperationIds.empty())
			{
				m_operationStack->setVisible(workflows[workflowIndex].OperationIds.size() > 1);
				const QString lastOperationId = m_lastOperationByWorkflowIndex.value(workflowIndex);
				SetSelectedOperation(workflows[workflowIndex].OperationIds.contains(lastOperationId) ? lastOperationId : workflows[workflowIndex].OperationIds.front());
			}
		}
	}

	void LauncherMainWindow::SelectProcessButton(QAbstractButton* button)
	{
		if (button == nullptr)
		{
			return;
		}

		SetSelectedOperation(button->property("OperationId").toString());
	}

	void LauncherMainWindow::DisplaySelectedRunOutput(QListWidgetItem* currentItem, QListWidgetItem*)
	{
		if (currentItem == nullptr)
		{
			return;
		}

		const QString runId = currentItem->data(Qt::UserRole).toString();
		ShowRunOutput(runId);
		const RunState state = m_runStates.value(runId, RunState::Done);
		if (state == RunState::Running || state == RunState::Failed)
		{
			SetActivityLogExpanded(true);
		}
	}

	void LauncherMainWindow::CopySelectedRunOutput()
	{
		if (m_operationOutput == nullptr)
		{
			return;
		}

		QGuiApplication::clipboard()->setText(m_operationOutput->toPlainText());
		SetStatusMessage("Copied selected run output");
	}

	void LauncherMainWindow::ToggleActivityLogPanel()
	{
		SetActivityLogExpanded(!m_activityLogExpanded);
	}

	void LauncherMainWindow::RunSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText("Choose a workflow before running.");
			}
			SetStatusMessage("No workflow selected");
			return;
		}

		if (m_selectedOperationId == LauncherHomeOperationId())
		{
			SetStatusMessage("Use Quick Start cards to launch, sync, build, cook, test, or package.");
			return;
		}

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString message = "No project discovered. Confirm this is a Sparkle repository or package root with Projects/<Project> markers.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			SetStatusMessage(message);
			return;
		}

		if ((m_selectedOperationId == "workspace.setup" || m_selectedOperationId == "workspace.generate-solution" || m_selectedOperationId == "workspace.open-solution" ||
		     m_selectedOperationId == "workspace.build-all" || m_selectedOperationId == "launcher.build.self" || m_selectedOperationId.startsWith("project.build") || m_selectedOperationId == "cook.tools.prepare") &&
		    !OfferWorkspacePrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (m_selectedOperationId.startsWith("cook.") && m_selectedOperationId != "cook.tools.prepare" && !OfferCookPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		if (FindLaunchOperationDefinition(m_selectedOperationId.toStdString()).has_value() && !OfferLaunchPrerequisiteOperation(m_selectedOperationId))
		{
			return;
		}

		LauncherOperationRequest request = BuildOperationRequest(m_selectedOperationId);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Run canceled");
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		StartOperation(std::move(request), title);
	}

	void LauncherMainWindow::CleanSelectedOperation()
	{
		if (m_selectedOperationId.isEmpty())
		{
			SetStatusMessage("No workflow selected");
			return;
		}

		if (!SupportsActionSpecificClean(m_selectedOperationId))
		{
			SetStatusMessage("Clean is not available for this workflow");
			return;
		}

		LauncherOperationRequest request = BuildCleanOperationRequest(m_selectedOperationId);
		if (request.CleanTargets.isEmpty())
		{
			const QString message = OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty() ?
			                            "Select a project before cleaning this workflow's generated outputs." :
			                            "No generated outputs were resolved for this workflow.";
			if (m_operationOutput != nullptr)
			{
				m_operationOutput->setPlainText(message);
			}
			SetStatusMessage(message);
			return;
		}

		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Clean canceled");
			return;
		}

		StartOperation(std::move(request), "Clean " + DisplayNameForOperation(m_selectedOperationId));
	}

	void LauncherMainWindow::DisplayOperationStarted(const QString& runId, const QString&, const QString& title)
	{
		const QString effectiveTitle = m_runTitles.value(runId, title);
		SetRunState(runId, RunState::Running, effectiveTitle);
		AppendRunOutput(runId, effectiveTitle + " started.\n");
		ShowRunOutput(runId);
		SetActivityLogExpanded(true);
		SetStatusMessage(effectiveTitle + " running");
		UpdateProgress();
	}

	void LauncherMainWindow::AppendOperationOutput(const QString& runId, const QString&, const QString& outputText)
	{
		AppendRunOutput(runId, outputText);
		if (m_activeRunId == runId)
		{
			ShowRunOutput(runId);
		}
	}

	void LauncherMainWindow::DisplayOperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode)
	{
		const bool succeeded = exitCode == 0;
		const QString effectiveTitle = m_runTitles.value(runId, title);
		SetRunState(runId, succeeded ? RunState::Done : RunState::Failed, effectiveTitle);

		if (succeeded)
		{
			AppendRunOutput(runId, "\n" + effectiveTitle + " finished: " + statusText + "\n");
		}
		else
		{
			const QString existingOutput = m_runOutputs.value(runId);
			const QString recoveryHint = FailureRecoveryHint(operationId, statusText);
			const QString recoveryText = recoveryHint.isEmpty() ? QString() : QStringLiteral("Recovery: %1\n\n").arg(recoveryHint);
			m_runOutputs.insert(
			    runId,
			    QStringLiteral("Failed: %1 (exit code %2)\n").arg(statusText).arg(exitCode) + recoveryText + "\n" + existingOutput + "\n" + effectiveTitle + " finished: " + statusText + "\n");
			++m_failedRunCount;
		}

		++m_finishedRunCount;
		ActionHistoryRecord historyRecord;
		historyRecord.CompletedAtUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
		historyRecord.ResultText = statusText;
		historyRecord.ExitCode = exitCode;
		m_actionHistory.insert(operationId, historyRecord);
		SaveActionHistory();
		UpdateActionHistoryDisplay();
		ShowRunOutput(runId);
		SetActivityLogExpanded(!succeeded);
		SetStatusMessage(effectiveTitle + " finished: " + statusText);
		UpdateProgress();
		RefreshProjects();
		RebuildOptionsPages();

		if (succeeded && operationId == "launcher.build.self" && !m_pendingRestartRunIds.contains(runId))
		{
			m_pendingRestartRunIds.push_back(runId);
			PromptForLauncherRestart();
		}

		if (m_pendingFollowUpOperations.contains(runId))
		{
			const PendingFollowUpOperation followUp = m_pendingFollowUpOperations.take(runId);
			if (succeeded)
			{
				StartOperation(followUp.Request, followUp.Title);
			}
			else
			{
				SetStatusMessage(followUp.Title + " canceled because the prerequisite clean failed.");
			}
		}
	}

	QWidget* LauncherMainWindow::CreateWorkflowSurface()
	{
		QFrame* surface = new QFrame(this);
		surface->setObjectName("WorkflowSurface");
		QHBoxLayout* layout = new QHBoxLayout(surface);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		layout->addWidget(CreateProcessPicker(surface), 0);
		layout->addWidget(CreateOptionsPanel(surface), 1);
		layout->addWidget(CreateOutputPanel(), 0);
		return surface;
	}

	QWidget* LauncherMainWindow::CreateProcessPicker(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("ProcessPanel");
		panel->setFixedWidth(kWorkflowRailWidth);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 10, 0, 10);
		layout->setSpacing(4);

		QVBoxLayout* groupLayout = new QVBoxLayout();
		groupLayout->setContentsMargins(0, 0, 0, 0);
		groupLayout->setSpacing(2);

		m_workflowGroupButtonGroup = new QButtonGroup(this);
		m_workflowGroupButtonGroup->setExclusive(true);
		m_processButtonGroup = new QButtonGroup(this);
		m_processButtonGroup->setExclusive(true);

		m_operationStack = new QStackedWidget(panel);
		m_operationStack->setObjectName("OperationStack");

		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		for (int workflowIndex = 0; workflowIndex < workflows.size(); ++workflowIndex)
		{
			const LauncherWorkflowDefinition& workflow = workflows[workflowIndex];
			QToolButton* groupButton = new QToolButton(panel);
			groupButton->setText(workflow.Title);
			groupButton->setObjectName("WorkflowGroupButton");
			groupButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
			groupButton->setMinimumHeight(kWorkflowGroupMinHeight);
			groupButton->setMaximumHeight(kWorkflowGroupMinHeight);
			groupButton->setMinimumWidth(kWorkflowRailWidth);
			groupButton->setMaximumWidth(kWorkflowRailWidth);
			groupButton->setProperty("WorkflowIndex", workflowIndex);
			groupButton->setProperty("ActiveState", "false");
			groupButton->setAccessibleName(workflow.Title + " workflow group");
			groupButton->setIcon(WorkflowIconForKey(workflow.IconKey));
			groupButton->setIconSize(QSize(18, 18));
			RegisterFocusable(groupButton);
			m_workflowGroupButtonGroup->addButton(groupButton);
			groupLayout->addWidget(groupButton);

			QWidget* tabPage = new QWidget(m_operationStack);
			QHBoxLayout* actionLayout = new QHBoxLayout();
			actionLayout->setContentsMargins(0, 0, 0, 0);
			actionLayout->setSpacing(18);
			if (workflow.OperationIds.size() > 1)
			{
				for (int index = 0; index < workflow.OperationIds.size(); ++index)
				{
					const QString& operationId = workflow.OperationIds[index];
					QPushButton* button = CreateProcessButton(DisplayNameForOperation(operationId), operationId, tabPage);
					m_processButtonGroup->addButton(button);
					actionLayout->addWidget(button);
				}
			}
			actionLayout->addStretch(1);
			tabPage->setLayout(actionLayout);
			const int pageIndex = m_operationStack->addWidget(tabPage);
			for (const QString& operationId : workflow.OperationIds)
			{
				m_workflowPageByOperation.insert(operationId, pageIndex);
			}
		}
		groupLayout->addStretch(1);
		connect(m_workflowGroupButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectWorkflowGroupButton);
		connect(m_processButtonGroup, &QButtonGroup::buttonClicked, this, &LauncherMainWindow::SelectProcessButton);
		layout->addLayout(groupLayout, 1);
		return panel;
	}

	QPushButton* LauncherMainWindow::CreateProcessButton(const QString& label, const QString& operationId, QWidget* parent)
	{
		QPushButton* button = new QPushButton(label, parent);
		button->setObjectName("WorkflowButton");
		button->setCheckable(true);
		button->setMinimumHeight(kWorkflowButtonMinHeight);
		button->setProperty("OperationId", operationId);
		button->setAccessibleName(label + " workflow");
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateOptionsPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("OptionsPanel");
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		QFrame* titleBand = new QFrame(panel);
		titleBand->setObjectName("TitleBand");
		QHBoxLayout* titleBandLayout = new QHBoxLayout(titleBand);
		titleBandLayout->setContentsMargins(18, 0, 12, 0);
		titleBandLayout->setSpacing(12);

		QVBoxLayout* titleStack = new QVBoxLayout();
		titleStack->setContentsMargins(0, 0, 0, 0);
		titleStack->setSpacing(0);
		m_activeOperationLabel = new QLabel("No workflow selected", titleBand);
		m_activeOperationLabel->setObjectName("ActiveOperationLabel");
		m_activeOperationLabel->setAccessibleName("Selected workflow");
		titleStack->addWidget(m_activeOperationLabel, 0, Qt::AlignVCenter);
		titleBandLayout->addLayout(titleStack, 1);

		QWidget* headerUtilities = CreateHeaderContextPanel(titleBand);
		if (headerUtilities != nullptr)
		{
			titleBandLayout->addWidget(headerUtilities, 0, Qt::AlignRight | Qt::AlignVCenter);
		}
		layout->addWidget(titleBand, 0);

		if (m_operationStack != nullptr)
		{
			m_operationStack->setParent(panel);
			layout->addWidget(m_operationStack, 0);
		}

		m_optionsStack = new QStackedWidget(panel);
		m_optionsStack->setObjectName("OptionsStack");
		RebuildOptionsPages();
		layout->addWidget(m_optionsStack, 1);
		m_optionsStack->setVisible(false);

		m_actionMetaPanel = new QFrame(panel);
		m_actionMetaPanel->setObjectName("ActionMetaPanel");
		QHBoxLayout* actionMetaRowLayout = new QHBoxLayout(m_actionMetaPanel);
		actionMetaRowLayout->setContentsMargins(0, 8, 0, 0);
		actionMetaRowLayout->setSpacing(8);

		QVBoxLayout* actionMetaLayout = new QVBoxLayout();
		actionMetaLayout->setContentsMargins(0, 0, 0, 0);
		actionMetaLayout->setSpacing(kSpaceTiny);
		QLabel* actionMetaTitle = new QLabel("Last completed run", m_actionMetaPanel);
		actionMetaTitle->setObjectName("ActionMetaTitle");
		actionMetaLayout->addWidget(actionMetaTitle);
		m_lastRunSummaryLabel = new QLabel("No recorded run for this workflow yet.", m_actionMetaPanel);
		m_lastRunSummaryLabel->setObjectName("ActionMetaText");
		m_lastRunSummaryLabel->setWordWrap(true);
		actionMetaLayout->addWidget(m_lastRunSummaryLabel);
		m_lastRunResultLabel = new QLabel("Result data will persist between launcher sessions.", m_actionMetaPanel);
		m_lastRunResultLabel->setObjectName("ActionMetaDetail");
		m_lastRunResultLabel->setWordWrap(true);
		actionMetaLayout->addWidget(m_lastRunResultLabel);
		actionMetaRowLayout->addLayout(actionMetaLayout, 1);

		m_cleanButton = new QPushButton("Clean", panel);
		m_cleanButton->setObjectName("SecondaryButton");
		m_cleanButton->setToolTip("Clean only the generated outputs tied to this action.");
		m_cleanButton->setEnabled(false);
		m_cleanButton->setAccessibleName("Clean selected workflow outputs");
		RegisterFocusable(m_cleanButton);
		connect(m_cleanButton, &QPushButton::clicked, this, &LauncherMainWindow::CleanSelectedOperation);
		actionMetaRowLayout->addWidget(m_cleanButton, 0, Qt::AlignRight | Qt::AlignVCenter);

		m_dismissHistoryButton = new QPushButton("Dismiss", panel);
		m_dismissHistoryButton->setObjectName("SecondaryButton");
		m_dismissHistoryButton->setToolTip("Hide the stored result summary for this workflow. Log files remain available.");
		m_dismissHistoryButton->setEnabled(false);
		m_dismissHistoryButton->setAccessibleName("Dismiss stored workflow result");
		RegisterFocusable(m_dismissHistoryButton);
		connect(m_dismissHistoryButton, &QPushButton::clicked, this, &LauncherMainWindow::DismissSelectedActionHistory);
		actionMetaRowLayout->addWidget(m_dismissHistoryButton, 0, Qt::AlignRight | Qt::AlignVCenter);

		m_runButton = new QPushButton("Run", panel);
		m_runButton->setObjectName("PrimaryActionButton");
		m_runButton->setIcon(CreateLauncherIcon(LauncherIcon::Run, QColor("#ffffff")));
		m_runButton->setIconSize(QSize(kLauncherIconSize, kLauncherIconSize));
		m_runButton->setToolTip("Run the selected workflow. Existing runs keep going.");
		m_runButton->setEnabled(false);
		m_runButton->setAccessibleName("Run selected workflow");
		RegisterFocusable(m_runButton);
		connect(m_runButton, &QPushButton::clicked, this, &LauncherMainWindow::RunSelectedOperation);
		actionMetaRowLayout->addWidget(m_runButton, 0, Qt::AlignRight | Qt::AlignVCenter);
		layout->addWidget(m_actionMetaPanel);
		UpdateActionHistoryDisplay();
		return panel;
	}

	QWidget* LauncherMainWindow::CreateHeaderContextPanel(QWidget* parent)
	{
		QFrame* panel = new QFrame(parent);
		panel->setObjectName("HeaderUtilityPanel");
		QHBoxLayout* rowLayout = new QHBoxLayout(panel);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(8);

		QLabel* projectLabel = CreateFieldLabel("Project");
		projectLabel->setObjectName("HeaderFieldLabel");
		rowLayout->addWidget(projectLabel, 0);
		QComboBox* projectCombo = CreateProjectCombo();
		projectCombo->setObjectName("HeaderContextCombo");
		projectCombo->setAccessibleName("Project");
		projectCombo->setToolTip("Global project context used by project, cook, launch, and smoke workflows.");
		projectCombo->setMinimumWidth(140);
		projectCombo->setMaximumWidth(180);
		projectCombo->setMinimumHeight(28);
		projectCombo->setMaximumHeight(28);
		projectLabel->setBuddy(projectCombo);
		rowLayout->addWidget(projectCombo, 0);

		QLabel* configurationLabel = CreateFieldLabel("Config");
		configurationLabel->setObjectName("HeaderFieldLabel");
		rowLayout->addWidget(configurationLabel, 0);
		QComboBox* configurationCombo = CreateValueCombo(
		    {{"Development", "development"}, {"Debug", "debug"}, {"Shipping", "shipping"}},
		    m_settings.BuildConfiguration(),
		    &LauncherSettings::SetBuildConfiguration);
		configurationCombo->setObjectName("HeaderContextCombo");
		configurationCombo->setAccessibleName("Build Configuration");
		configurationCombo->setToolTip("Global build configuration used for editor, runtime, and tool workflows.");
		configurationCombo->setMinimumWidth(140);
		configurationCombo->setMaximumWidth(180);
		configurationCombo->setMinimumHeight(28);
		configurationCombo->setMaximumHeight(28);
		configurationLabel->setBuddy(configurationCombo);
		rowLayout->addWidget(configurationCombo, 0);

		QLabel* ideLabel = CreateFieldLabel("IDE");
		ideLabel->setObjectName("HeaderFieldLabel");
		rowLayout->addWidget(ideLabel, 0);
		QComboBox* ideCombo = CreateValueCombo({{"Visual Studio", "visual-studio"}, {"Rider", "rider"}}, m_settings.WorkspaceIde(), &LauncherSettings::SetWorkspaceIde);
		ideCombo->setObjectName("HeaderContextCombo");
		ideCombo->setAccessibleName("IDE");
		ideCombo->setToolTip("Visual Studio with an MSVC-compatible Qt kit is the supported Windows workflow. ClangCL remains supported as an optional toolset, and Rider remains optional IDE integration.");
		ideCombo->setMinimumWidth(120);
		ideCombo->setMaximumWidth(150);
		ideCombo->setMinimumHeight(28);
		ideCombo->setMaximumHeight(28);
		ideLabel->setBuddy(ideCombo);
		rowLayout->addWidget(ideCombo, 0);

		rowLayout->addSpacing(6);

		QWidget* folderShortcuts = CreateFolderShortcutActions();
		if (folderShortcuts != nullptr)
		{
			folderShortcuts->setParent(panel);
			rowLayout->addWidget(folderShortcuts, 0, Qt::AlignLeft | Qt::AlignVCenter);
		}

		QPushButton* activityButton = new QPushButton("Activity", panel);
		activityButton->setObjectName("HeaderUtilityButton");
		activityButton->setToolTip("Open recent run activity and raw logs.");
		activityButton->setAccessibleName("Open activity drawer");
		activityButton->setMinimumHeight(28);
		activityButton->setMaximumHeight(30);
		RegisterFocusable(activityButton);
		connect(activityButton, &QPushButton::clicked, this, &LauncherMainWindow::ToggleActivityLogPanel);
		rowLayout->addWidget(activityButton, 0, Qt::AlignLeft | Qt::AlignVCenter);

		m_headerContextPanel = panel;
		return panel;
	}

	QWidget* LauncherMainWindow::CreateOptionsPage(const QString& operationId, QWidget* parent)
	{
		QScrollArea* scrollArea = new QScrollArea(parent);
		scrollArea->setObjectName("OptionsScrollArea");
		scrollArea->setWidgetResizable(true);
		scrollArea->setFrameShape(QFrame::NoFrame);
		scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

		QWidget* content = new QWidget(scrollArea);
		content->setObjectName("OptionsContent");
		content->setMaximumWidth(1320);
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(24, 18, 24, 28);
		layout->setSpacing(8);
		AddOptionsForOperation(*layout, operationId);
		layout->addStretch(1);
		scrollArea->setWidget(content);
		return scrollArea;
	}

	QWidget* LauncherMainWindow::CreateOutputPanel()
	{
		const LauncherOutputPanelWidgets widgets = CreateLauncherOutputPanel(
		    this,
		    CreateLauncherIcon(LauncherIcon::Copy, QColor(kColorStateQueued)),
		    QSize(kLauncherIconSize, kLauncherIconSize),
		    [this](QWidget* widget) { RegisterFocusable(widget); },
		    [this]() { ToggleActivityLogPanel(); },
		    [this]() { CopySelectedRunOutput(); },
		    [this](QListWidgetItem* current, QListWidgetItem* previous) { DisplaySelectedRunOutput(current, previous); });

		if (widgets.Root != nullptr)
		{
			widgets.Root->setObjectName("ActivityDrawer");
			widgets.Root->setMinimumWidth(360);
			widgets.Root->setMaximumWidth(460);
		}
		m_activityDetailsPanel = widgets.ActivityDetailsPanel;
		m_activityList = widgets.ActivityList;
		m_selectedRunSummary = widgets.SelectedRunSummary;
		m_operationOutput = widgets.OperationOutput;
		m_toggleOutputButton = widgets.ToggleOutputButton;
		m_copyOutputButton = widgets.CopyOutputButton;
		m_progressLabel = widgets.ProgressLabel;
		if (m_operationOutput != nullptr)
		{
			m_operationOutput->setMinimumHeight(kOperationOutputMinHeight);
			m_operationOutput->setMaximumHeight(kOperationOutputMaxHeight);
		}
		SetActivityLogExpanded(false);
		return widgets.Root;
	}

	QLabel* LauncherMainWindow::CreateSectionLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("SectionLabel");
		label->setAccessibleName(title);
		return label;
	}

	QLabel* LauncherMainWindow::CreateFieldLabel(const QString& title) const
	{
		QLabel* label = new QLabel(title);
		label->setObjectName("FieldLabel");
		label->setAccessibleName(title);
		return label;
	}

	QCheckBox* LauncherMainWindow::CreateBoundCheckBox(const QString& label, const QString& tooltip, bool checked, void (LauncherSettings::*setter)(bool))
	{
		QCheckBox* box = new QCheckBox(label, this);
		box->setToolTip(tooltip);
		box->setAccessibleName(label);
		box->setAccessibleDescription(tooltip);
		box->setChecked(checked);
		RegisterFocusable(box);
		connect(box, &QCheckBox::toggled, &m_settings, setter);
		return box;
	}

	QLineEdit* LauncherMainWindow::CreateBoundLineEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&))
	{
		QLineEdit* edit = new QLineEdit(this);
		edit->setText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		RegisterFocusable(edit);
		connect(edit, &QLineEdit::textChanged, &m_settings, setter);
		return edit;
	}

	QTextEdit* LauncherMainWindow::CreateBoundTextEdit(const QString& text, const QString& placeholder, const QString& tooltip, void (LauncherSettings::*setter)(const QString&))
	{
		QTextEdit* edit = new QTextEdit(this);
		edit->setPlainText(text);
		edit->setPlaceholderText(placeholder);
		edit->setToolTip(tooltip);
		edit->setAccessibleDescription(tooltip);
		edit->setMinimumHeight(78);
		edit->setMaximumHeight(118);
		RegisterFocusable(edit);
		connect(edit, &QTextEdit::textChanged, this, [edit, setter, this]() {
			(m_settings.*setter)(edit->toPlainText());
		});
		return edit;
	}

	QComboBox* LauncherMainWindow::CreateProfileCombo(const QStringList& profiles, const QString& currentProfile, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->addItems(profiles);
		combo->setAccessibleName("Profile");
		combo->setAccessibleDescription("Build profile used by this workflow.");
		combo->setCurrentText(currentProfile);
		RegisterFocusable(combo);
		connect(combo, &QComboBox::currentTextChanged, &m_settings, setter);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateProjectCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setObjectName("ProjectCombo");
		combo->setProperty("ProjectSelector", true);
		combo->setToolTip("Project used by this workflow.");
		combo->setAccessibleName("Project");
		combo->setAccessibleDescription("Project used by this workflow.");
		RegisterFocusable(combo);
		m_projectSelectors.push_back(combo);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, this]() {
			const QString projectId = combo->currentData().toString();
			if (!projectId.isEmpty())
			{
				m_projectModel.SelectProject(projectId);
				SetStatusMessage("Selected project: " + combo->currentText());
			}
		});
		PopulateProjectCombo(*combo);
		return combo;
	}

	QComboBox* LauncherMainWindow::CreateValueCombo(const QVector<QPair<QString, QString>>& options, const QString& currentValue, void (LauncherSettings::*setter)(const QString&))
	{
		QComboBox* combo = new QComboBox(this);
		combo->setAccessibleName("Option value");
		RegisterFocusable(combo);
		for (const QPair<QString, QString>& option : options)
		{
			combo->addItem(option.first, option.second);
		}
		const int currentIndex = combo->findData(currentValue);
		combo->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, setter, this]() {
			(m_settings.*setter)(combo->currentData().toString());
		});
		return combo;
	}

	QFrame* LauncherMainWindow::CreateSourceTierCard(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		const int readyCount = CountReadyDependencies(group, dependencyCachePath);
		const QString state = DependencyGroupStatusState(group, readyCount);
		QFrame* card = new QFrame(this);
		card->setObjectName("SourceTierCard");
		card->setProperty("State", state);
		card->setMinimumHeight(138);
		QVBoxLayout* cardLayout = new QVBoxLayout(card);
		cardLayout->setContentsMargins(16, 14, 16, 14);
		cardLayout->setSpacing(8);

		QHBoxLayout* titleRow = new QHBoxLayout();
		titleRow->setContentsMargins(0, 0, 0, 0);
		titleRow->setSpacing(kSpaceSmall);
		QLabel* title = new QLabel(group.Label, card);
		title->setObjectName("SourceTierTitle");
		titleRow->addWidget(title, 1);
		QLabel* chip = new QLabel(DependencyGroupStatusText(group, readyCount), card);
		chip->setObjectName("SourceTierChip");
		chip->setProperty("State", state);
		titleRow->addWidget(chip, 0, Qt::AlignRight | Qt::AlignTop);
		cardLayout->addLayout(titleRow);

		QLabel* summary = new QLabel(group.Enabled ? group.UnlockSummary : FormatDependencyGroupDetail(group, dependencyCachePath, readyCount), card);
		summary->setObjectName("SourceTierText");
		summary->setWordWrap(true);
		cardLayout->addWidget(summary, 1);

		QHBoxLayout* metaRow = new QHBoxLayout();
		metaRow->setContentsMargins(0, 0, 0, 0);
		metaRow->setSpacing(kSpaceSmall);
		const QString metaText = group.Required ? "Required" : (group.Enabled ? "Optional enabled" : "Optional disabled");
		QLabel* meta = new QLabel(metaText, card);
		meta->setObjectName("SourceTierMeta");
		metaRow->addWidget(meta, 1);
		if (group.Enabled)
		{
			QWidget* actions = CreateActionDependencyActions("workspace.setup", "Sync Source Tiers", "deps", "Clean Source Dependency Cache");
			if (actions != nullptr)
			{
				actions->setParent(card);
				metaRow->addWidget(actions, 0, Qt::AlignRight);
			}
		}
		else
		{
			QWidget* actions = CreateDisabledSourceTierActions(group);
			if (actions != nullptr)
			{
				actions->setParent(card);
				metaRow->addWidget(actions, 0, Qt::AlignRight);
			}
		}
		cardLayout->addLayout(metaRow);
		return card;
	}

	void LauncherMainWindow::AddSourceTierCards(QVBoxLayout& layout, const QString& title, const QString& detail, bool includeDependencyDetails)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		QVBoxLayout* tiersLayout = AddOptionGroup(layout, title, detail);
		QGridLayout* tierGrid = new QGridLayout();
		tierGrid->setContentsMargins(0, 4, 0, 0);
		tierGrid->setHorizontalSpacing(12);
		tierGrid->setVerticalSpacing(12);
		int index = 0;
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			tierGrid->addWidget(CreateSourceTierCard(group, dependencyCachePath), index / 2, index % 2);
			++index;
		}
		tiersLayout->addLayout(tierGrid);

		if (!includeDependencyDetails)
		{
			return;
		}

		QVBoxLayout* inventoryLayout = AddDetailsGroup(
		    layout,
		    "Dependency Inventory",
		    "Individual source dependencies and cache actions. Keep this closed unless a specific cache needs inspection or repair.",
		    false);
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			QVBoxLayout* dependenciesLayout = AddDetailsGroup(
			    *inventoryLayout,
			    group.Label + " Contents",
			    group.Enabled ? group.UnlockSummary : FormatDependencyGroupDetail(group, dependencyCachePath, 0),
			    false);
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				const std::filesystem::path dependencyPath = dependencyCachePath / dependency.CacheDirectoryName.toStdString();
				const bool dependencyReady = DirectoryHasEntries(dependencyPath);
				AddStatusRow(
				    *dependenciesLayout,
				    QStringLiteral("%1 (%2)").arg(dependency.Label, dependency.Version),
				    !group.Enabled ? "Disabled" : dependencyReady ? "Cached" : "Pending sync",
				    FormatDependencyEntryDetail(group, dependency, dependencyPath),
				    !group.Enabled ? "neutral" : dependencyReady ? "ok" : "warning",
				    group.Enabled ? CreateTrackedDependencyActions(dependency) : CreateDisabledSourceTierActions(group));
			}
		}
	}

	void LauncherMainWindow::AddOptionsForOperation(QVBoxLayout& layout, const QString& operationId)
	{
		if (operationId == LauncherHomeOperationId())
		{
			AddHomeQuickStart(layout);
			return;
		}

		if (operationId == LauncherSystemOperationId())
		{
			AddSystemOverviewPage(layout);
			return;
		}

		AddWorkflowPageHeader(layout, operationId);

		if (operationId == "package.release")
		{
			QVBoxLayout* packageLayout = AddOptionGroup(
			    layout,
			    "Package Assembly",
			    "Assemble a release package layout while keeping final validation and publishing sign-off separate.");
			AddStatusRow(
			    *packageLayout,
			    "Release package",
			    "Assembly target",
			    "Build the sparkle_release_assembly CMake target to assemble launcher, editor/runtime, cooked content, manifests, checksums, notes, licenses, and a separate symbols archive under dist/releases/<version>.",
			    "neutral");
			AddStatusRow(
			    *packageLayout,
			    "Validation",
			    "Separate sign-off",
			    "This workflow assembles dist/ packages. Publish readiness still requires the final validation checklist and release report.",
			    "neutral");
			QVBoxLayout* contentsLayout = AddDetailsGroup(
			    layout,
			    "Selection Details",
			    "Package inclusion follows product ownership, visibility, binary type, declared dependencies, and package navigation rules.",
			    false);
			AddStatusRow(*contentsLayout, "Launcher", "Included", "Package-root SparkleLauncher.exe and runtime support files.", "neutral");
			AddStatusRow(*contentsLayout, "Showcase products", "Staged when present", "Showcase editor/runtime binaries and cooked Showcase content are staged from artifacts.", "neutral");
			AddStatusRow(*contentsLayout, "Manifests", "Generated", "Release, build, dependency, bundled-runtime, file hash, checksum, and notes outputs.", "neutral");
			AddStatusRow(*contentsLayout, "Symbols", "Separate archive", "Debug symbols stay outside user-facing runtime packages.", "neutral");
			return;
		}

		if (operationId == "workspace.generate-solution" || operationId == "workspace.open-solution" || operationId == "toolchain.check" || operationId == "workspace.setup")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "workspace.build-all")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.editor")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "launcher.build.self")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.build.runtime")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.tools.prepare")
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "cook.shaders")
		{
			AddBuildEnvironmentStatus(layout, operationId);

			QVBoxLayout* selectionLayout = AddOptionGroup(layout, "Options", "Shader cook target selection. Advanced cache, debug, and compiler controls are available below.");
			AddOptionField(*selectionLayout, "Shader package", CreateValueCombo(
			    {{"All shader packages", ""},
			     {"ComputeClear", "ComputeClear"},
			     {"DirectLighting", "DirectLighting"},
			     {"GBuffer", "GBuffer"},
			     {"HelloInlineRayQuery", "HelloInlineRayQuery"},
			     {"HelloRayTracingLibrary", "HelloRayTracingLibrary"},
			     {"HelloTriangle", "HelloTriangle"},
			     {"IndirectLighting", "IndirectLighting"},
			     {"LightingComposite", "LightingComposite"},
			     {"Sky", "Sky"},
			     {"VisualizeBuffers", "VisualizeBuffers"}},
			    m_settings.ShaderPackages(),
			    &LauncherSettings::SetShaderPackages));
			AddOptionField(*selectionLayout, "Compiler backend", CreateValueCombo(
			    {{"Auto select", "auto"}, {"DXC", "dxc"}, {"Slang", "slang"}},
			    m_settings.ShaderBackend(),
			    &LauncherSettings::SetShaderBackend));
			QComboBox* targetPresetCombo = CreateValueCombo(
			    {{"Default runtime set (DxilSm66 + SpirV16)", "default"},
			     {"DirectX 12 only (DxilSm66)", "d3d12"},
			     {"Vulkan only (SpirV16)", "vulkan"},
			     {"DXIL compatibility sweep (Sm60-Sm67)", "dxil-all"},
			     {"SPIR-V compatibility sweep (1.4-1.6)", "spirv-all"},
			     {"Custom target list", "custom"}},
			    m_settings.ShaderTargetPreset(),
			    &LauncherSettings::SetShaderTargetPreset);
			AddOptionField(*selectionLayout, "Binary targets", targetPresetCombo);
			QWidget* customTargetsRow = AddOptionField(
			    *selectionLayout,
			    "Custom targets",
			    CreateBoundLineEdit(
			        m_settings.ShaderCustomTargets(),
			        "DxilSm66, SpirV16",
			        "Comma-separated ShaderCompiler target names such as DxilSm66 or SpirV16.",
			        &LauncherSettings::SetShaderCustomTargets));
			customTargetsRow->setVisible(m_settings.ShaderTargetPreset() == "custom");
			connect(targetPresetCombo, &QComboBox::currentTextChanged, customTargetsRow, [this, customTargetsRow](const QString&) {
				customTargetsRow->setVisible(m_settings.ShaderTargetPreset() == "custom");
			});

			QVBoxLayout* advancedLayout = AddDetailsGroup(layout, "Advanced Shader Options", "Cache, diagnostics, and compiler-output controls for shader investigation and production tuning.", false);
			advancedLayout->addWidget(CreateSectionLabel("Cache And Outputs"));
			AddOptionCheckBox(*advancedLayout, CreateBoundCheckBox("Use shader cache", "Reuse cached shader compile artifacts when possible.", m_settings.ShaderUseCache(), &LauncherSettings::SetShaderUseCache));
			AddOptionField(
			    *advancedLayout,
			    "Cache directory",
			    CreateBoundLineEdit(
			        m_settings.ShaderCacheDirectory(),
			        "Use ShaderCompiler default cache location",
			        "Optional override for ShaderCompiler --cache-dir.",
			        &LauncherSettings::SetShaderCacheDirectory));
			QCheckBox* debugArtifactsBox = CreateBoundCheckBox(
			    "Write debug artifact bundles",
			    "Emit compiler-side debug bundles and intermediate artifacts for inspection.",
			    m_settings.ShaderWriteDebugArtifacts(),
			    &LauncherSettings::SetShaderWriteDebugArtifacts);
			debugArtifactsBox->setObjectName("WarningCheckBox");
			AddOptionCheckBox(*advancedLayout, debugArtifactsBox);
			QWidget* debugArtifactsRow = AddOptionField(
			    *advancedLayout,
			    "Debug output directory",
			    CreateBoundLineEdit(
			        m_settings.ShaderDebugArtifactDirectory(),
			        ResolvedShaderDebugArtifactDirectory(m_repositoryRoot, m_projectModel, m_settings),
			        "Optional override for ShaderCompiler --debug-artifacts. When empty, the launcher uses a build-local default directory.",
			        &LauncherSettings::SetShaderDebugArtifactDirectory));
			debugArtifactsRow->setVisible(m_settings.ShaderWriteDebugArtifacts());
			connect(debugArtifactsBox, &QCheckBox::toggled, debugArtifactsRow, &QWidget::setVisible);

			advancedLayout->addWidget(CreateSectionLabel("Diagnostics"));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Enable debug info and symbols",
			        "Request backend debug information and symbol emission where the selected backend supports it.",
			        m_settings.ShaderEnableDebugInfo(),
			        &LauncherSettings::SetShaderEnableDebugInfo));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Enable compiler optimizations",
			        "Compile shaders with backend optimizations enabled. Disable when debugging compiler output or reproducing optimization-sensitive issues.",
			        m_settings.ShaderEnableOptimizations(),
			        &LauncherSettings::SetShaderEnableOptimizations));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Treat warnings as errors",
			        "Fail the shader cook when the backend emits warnings.",
			        m_settings.ShaderWarningsAsErrors(),
			        &LauncherSettings::SetShaderWarningsAsErrors));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Strip reflection from runtime binaries",
			        "Request reflection stripping for final runtime shader binaries where the active backend supports it.",
			        m_settings.ShaderStripReflection(),
			        &LauncherSettings::SetShaderStripReflection));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Strip embedded debug info from runtime binaries",
			        "Request embedded debug info stripping for final runtime shader binaries where the active backend supports it.",
			        m_settings.ShaderStripDebugInfo(),
			        &LauncherSettings::SetShaderStripDebugInfo));
			AddOptionCheckBox(
			    *advancedLayout,
			    CreateBoundCheckBox(
			        "Write cooked shader stats CSV",
			        "Run the cooked-shader-stats analysis pass after the shader cook and write CSV output into the shader cache analysis folder.",
			        m_settings.ShaderWriteCookedShaderStats(),
			        &LauncherSettings::SetShaderWriteCookedShaderStats));
			return;
		}

		if (operationId.startsWith("cook."))
		{
			AddBuildEnvironmentStatus(layout, operationId);
			return;
		}

		if (operationId == "project.open.editor" || operationId == "project.open.runtime")
		{
			AddLaunchEnvironmentStatus(layout, operationId);
			AddLaunchApplicationOptions(layout);
			return;
		}

		if (operationId == "project.run.smoke")
		{
			AddLaunchEnvironmentStatus(layout, operationId);
			AddLaunchTargetOptions(layout, "Smoke Target", "Choose which project executable should run with smoke validation.");
			AddLaunchApplicationOptions(layout);
			AddSmokeValidationOptions(layout);
			QVBoxLayout* gateLayout = AddDetailsGroup(
			    layout,
			    "Quality Gate Coverage",
			    "Real launcher-backed checks are shown here. Additional gates should appear only after the repo has a real runner or preset behind them.",
			    false);
			AddStatusRow(*gateLayout, "Smoke validation", "Available", "Runs the selected editor or runtime with smoke validation enabled.", "ok");
			AddStatusRow(*gateLayout, "Sanitizer builds", "Configured in CMake", "Root CMake exposes ENABLE_SANITIZERS and SANITIZER_TYPE for Clang-based ASan, UBSan, TSan, MSan, and LSan builds; launcher presets still need a dedicated workflow before this becomes one-click.", "neutral");
			AddStatusRow(*gateLayout, "Unit / integration / coverage", "Not wired", "No launcher-backed test or coverage runner is currently configured. Add a real CTest/coverage pipeline before exposing executable buttons here.", "neutral");
			return;
		}

		if (operationId == "project.run")
		{
			AddLaunchEnvironmentStatus(layout, operationId);
			AddLaunchTargetOptions(layout, "Launch Target", "Choose whether this launch starts the editor or runtime.");
			AddLaunchApplicationOptions(layout);
			return;
		}

		if (operationId == "quality.format")
		{
			AddMaintenanceEnvironmentStatus(layout, operationId);
			QVBoxLayout* formatOptionsLayout = AddOptionGroup(layout, "Formatting Mode", "Run clang-format as a quality gate or explicitly apply formatting changes.");
			AddOptionField(
			    *formatOptionsLayout,
			    "Mode",
			    CreateValueCombo(
			        {{"Check only", "check"}, {"Apply formatting", "apply"}},
			        m_settings.FormatMode(),
			        &LauncherSettings::SetFormatMode));
			return;
		}

		if (operationId == "workspace.clean")
		{
			AddMaintenanceEnvironmentStatus(layout, operationId);

			const std::array<CleanScopeUiOption, 7> cleanScopes = {{
			    {"Selected project cooked content", "selected-cooked", "Cooked asset outputs for only the selected project under artifacts/dev/projects/<Project>/cooked.", QString(), "Cooked content"},
			    {"All cooked content", "all-cooked", "Cooked asset domains for every project plus the shared cooked domain. Keeps editor/runtime artifacts and source dependency caches.", "artifacts/dev/projects/*/cooked", "Cooked content"},
			    {"Build outputs and generated build files", "build-tree", "Build outputs, intermediates, generated CMake/Visual Studio files, and local IDE state. Keeps the source dependency cache.", "build content except build/_deps, .vs, root generated project files, project generated files", "Build state"},
			    {"Shader cache", "shader-cache", "Transient shader cache, recook signal, debug artifacts, and shader outputs.", QString(), "Caches"},
			    {"Source dependency cache", "deps", "Downloaded source dependency cache. Configure will re-download source dependency groups.", QString(), "Caches"},
			    {"Logs", "logs", "Repository, launcher, and project logs.", "logs, artifacts/dev/launcher-state/Logs, Projects/*/logs", "Logs"},
			    {"Reset generated workspace", "pristine", "All generated workspace state, including build trees, artifacts, packages, dependency cache, cooked data, IDE state, logs, and generated project files. Close the launcher for absolute pristine cleanup of its live artifact copy.", "build, artifacts, dist, .vs, .vscode, logs, imgui.ini, root generated project files, project generated files", "Reset"},
			}};

			QVector<QCheckBox*> scopeBoxes;
			const QString selectedProjectId = m_projectModel.SelectedProjectId();
			const QStringList selectedScopes = m_settings.CleanScope().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
			const std::array<QPair<QString, QString>, 5> cleanGroups = {{
			    {"Cooked content", "Remove cooked assets for one project or every project."},
			    {"Build state", "Remove generated build products and local IDE workspace state."},
			    {"Caches", "Remove caches that will be recreated by later workflows."},
			    {"Logs", "Remove repository, launcher, and project logs."},
			    {"Reset", "Clear nearly all generated workspace state in one pass."},
			}};

			QVBoxLayout* planLayout = AddOptionGroup(
			    layout,
			    "Cleanup Plan",
			    "Select exactly the generated state you want to remove. The plan separates what will be cleaned from what will be kept before you run anything.");
			QLabel* selectedPlanLabel = new QLabel(planLayout->parentWidget());
			selectedPlanLabel->setObjectName("CleanPlanText");
			selectedPlanLabel->setWordWrap(true);
			planLayout->addWidget(selectedPlanLabel);
			QLabel* keptPlanLabel = new QLabel(planLayout->parentWidget());
			keptPlanLabel->setObjectName("CleanPlanText");
			keptPlanLabel->setWordWrap(true);
			planLayout->addWidget(keptPlanLabel);

			const auto addCleanScopeRow = [this, &scopeBoxes, &selectedProjectId, &selectedScopes](QGridLayout& groupGrid, const CleanScopeUiOption& scope, int row, int column) {
				QCheckBox* scopeBox = new QCheckBox(scope.Label, this);
				scopeBox->setToolTip(scope.Detail);
				scopeBox->setProperty("CleanScope", scope.Value);
				scopeBox->setProperty("CleanLabel", scope.Label);
				scopeBox->setChecked(selectedScopes.contains(scope.Value) || (selectedScopes.empty() && scope.Value == "selected-cooked"));
				RegisterFocusable(scopeBox);

				QFrame* scopeRow = new QFrame(this);
				scopeRow->setObjectName("CleanScopeCard");
				QVBoxLayout* scopeRowLayout = new QVBoxLayout(scopeRow);
				scopeRowLayout->setContentsMargins(12, 10, 12, 10);
				scopeRowLayout->setSpacing(6);
				scopeRowLayout->addWidget(scopeBox);
				const std::filesystem::path previewPath = ResolveCleanScopePreviewPath(m_repositoryRoot, selectedProjectId, scope.Value);
				const QString previewText = scope.Preview.isEmpty() ? ToDisplayPath(m_repositoryRoot, previewPath) + " - " + FormatDirectoryInventory(previewPath) : scope.Preview;
				QLabel* scopeDetail = new QLabel(previewText, scopeRow);
				scopeDetail->setObjectName("OptionHelpText");
				scopeDetail->setWordWrap(true);
				scopeRowLayout->addWidget(scopeDetail);
				groupGrid.addWidget(scopeRow, row, column);
				scopeBoxes.push_back(scopeBox);
			};

			for (const QPair<QString, QString>& cleanGroup : cleanGroups)
			{
				QVBoxLayout* cleanGroupLayout = AddOptionGroup(layout, cleanGroup.first, cleanGroup.second);
				QGridLayout* cleanGrid = new QGridLayout();
				cleanGrid->setContentsMargins(0, 4, 0, 0);
				cleanGrid->setHorizontalSpacing(12);
				cleanGrid->setVerticalSpacing(12);
				int groupScopeIndex = 0;
				for (const CleanScopeUiOption& scope : cleanScopes)
				{
					if (scope.Group == cleanGroup.first)
					{
						addCleanScopeRow(*cleanGrid, scope, groupScopeIndex / 2, groupScopeIndex % 2);
						++groupScopeIndex;
					}
				}
				cleanGroupLayout->addLayout(cleanGrid);
			}

			const auto updateCleanScopeSetting = [scopeBoxes, selectedPlanLabel, keptPlanLabel, this]() {
				QStringList selectedValues;
				QStringList selectedLabels;
				QStringList keptLabels;
				for (QCheckBox* scopeBox : scopeBoxes)
				{
					if (scopeBox != nullptr && scopeBox->isChecked())
					{
						selectedValues.push_back(scopeBox->property("CleanScope").toString());
						selectedLabels.push_back(scopeBox->property("CleanLabel").toString());
					}
					else if (scopeBox != nullptr)
					{
						keptLabels.push_back(scopeBox->property("CleanLabel").toString());
					}
				}
				if (selectedValues.empty())
				{
					if (!scopeBoxes.empty() && scopeBoxes.front() != nullptr)
					{
						const QSignalBlocker blocker(scopeBoxes.front());
						scopeBoxes.front()->setChecked(true);
					}
					selectedValues.push_back("selected-cooked");
					selectedLabels.push_back("Selected project cooked content");
					if (!keptLabels.empty())
					{
						keptLabels.removeAll("Selected project cooked content");
					}
				}
				selectedPlanLabel->setText("Will clean: " + selectedLabels.join(", "));
				keptPlanLabel->setText("Will keep: " + (keptLabels.isEmpty() ? QString("No generated scopes outside the selected plan.") : keptLabels.join(", ")));
				m_settings.SetCleanScope(selectedValues.join(';'));
				UpdateRunAvailability();
			};
			for (QCheckBox* scopeBox : scopeBoxes)
			{
				connect(scopeBox, &QCheckBox::toggled, this, updateCleanScopeSetting);
			}
			updateCleanScopeSetting();
			return;
		}

		AddNoOptionsMessage(layout, "No settings");
	}

	QWidget* LauncherMainWindow::AddOptionField(QVBoxLayout& layout, const QString& label, QWidget* control)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(0);

		QFrame* labelCell = new QFrame(row);
		labelCell->setObjectName("OptionLabelCell");
		QHBoxLayout* labelLayout = new QHBoxLayout(labelCell);
		labelLayout->setContentsMargins(10, 0, 10, 0);
		labelLayout->setSpacing(0);

		QLabel* fieldLabel = CreateFieldLabel(labelCell ? label : label);
		fieldLabel->setAlignment(Qt::AlignLeft | (qobject_cast<QTextEdit*>(control) != nullptr ? Qt::AlignTop : Qt::AlignVCenter));
		fieldLabel->setBuddy(control);
		labelLayout->addWidget(fieldLabel);
		labelCell->setFixedWidth(kFieldLabelWidth + 16);

		QFrame* valueCell = new QFrame(row);
		valueCell->setObjectName("OptionValueCell");
		QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
		valueLayout->setContentsMargins(0, 0, 0, 0);
		valueLayout->setSpacing(0);
		if (control->accessibleName().isEmpty() || control->accessibleName() == "Option value")
		{
			control->setAccessibleName(label);
		}
		if (control->toolTip().isEmpty())
		{
			control->setToolTip("Choose " + label.toLower() + " for this workflow.");
		}
		valueLayout->addWidget(control, 1);
		rowLayout->addWidget(labelCell, 0);
		rowLayout->addWidget(valueCell, 1);
		layout.addWidget(row);
		return row;
	}

	QWidget* LauncherMainWindow::AddOptionCheckBox(QVBoxLayout& layout, QCheckBox* checkBox)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("OptionRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(0);

		QFrame* labelCell = new QFrame(row);
		labelCell->setObjectName("OptionLabelCell");
		labelCell->setFixedWidth(kFieldLabelWidth + 16);

		QFrame* valueCell = new QFrame(row);
		valueCell->setObjectName("OptionValueCell");
		QHBoxLayout* valueLayout = new QHBoxLayout(valueCell);
		valueLayout->setContentsMargins(10, 0, 0, 0);
		valueLayout->setSpacing(0);
		valueLayout->addWidget(checkBox, 1);

		rowLayout->addWidget(labelCell, 0);
		rowLayout->addWidget(valueCell, 1);
		layout.addWidget(row);
		return row;
	}

	QVBoxLayout* LauncherMainWindow::AddOptionGroup(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(0, 8, 0, 8);
		groupLayout->setSpacing(4);

		QLabel* titleLabel = new QLabel(title, group);
		titleLabel->setObjectName("OptionGroupTitle");
		groupLayout->addWidget(titleLabel);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, group);
			detailLabel->setObjectName("OptionHelpText");
			detailLabel->setWordWrap(true);
			groupLayout->addWidget(detailLabel);
		}

		layout.addWidget(group);
		return groupLayout;
	}

	QVBoxLayout* LauncherMainWindow::AddDetailsGroup(QVBoxLayout& layout, const QString& title, const QString& detail, bool expanded)
	{
		QFrame* group = new QFrame(this);
		group->setObjectName("OptionGroup");
		QVBoxLayout* groupLayout = new QVBoxLayout(group);
		groupLayout->setContentsMargins(0, 8, 0, 8);
		groupLayout->setSpacing(4);

		QToolButton* toggle = new QToolButton(group);
		toggle->setObjectName("DetailsToggleButton");
		toggle->setText(title);
		toggle->setCheckable(true);
		toggle->setChecked(expanded);
		toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
		RegisterFocusable(toggle);
		groupLayout->addWidget(toggle);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, group);
			detailLabel->setObjectName("OptionHelpText");
			detailLabel->setWordWrap(true);
			groupLayout->addWidget(detailLabel);
		}

		QFrame* detailsPanel = new QFrame(group);
		detailsPanel->setObjectName("DetailsPanel");
		QVBoxLayout* detailsLayout = new QVBoxLayout(detailsPanel);
		detailsLayout->setContentsMargins(0, 4, 0, 0);
		detailsLayout->setSpacing(4);
		detailsPanel->setVisible(expanded);
		groupLayout->addWidget(detailsPanel);

		connect(toggle, &QToolButton::toggled, detailsPanel, [toggle, detailsPanel](bool checked) {
			toggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
			detailsPanel->setVisible(checked);
		});

		layout.addWidget(group);
		return detailsLayout;
	}

	void LauncherMainWindow::AddWorkflowPageHeader(QVBoxLayout& layout, const QString& operationId)
	{
		const auto history = m_actionHistory.constFind(operationId);
		if (history != m_actionHistory.constEnd() && history->ExitCode != 0)
		{
			const QString recoveryHint = FailureRecoveryHint(operationId, history->ResultText);
			const HomeNextAction recoveryAction = RecoveryActionForFailure(operationId, history->ResultText);
			QVBoxLayout* recoveryLayout = AddDetailsGroup(
			    layout,
			    "Current Workflow Recovery",
			    "Only shown when the selected workflow has a failed run. Raw logs stay in Activity.",
			    true);
			AddStatusRow(
			    *recoveryLayout,
			    "Current workflow recovery",
			    "Needs attention",
			    CombineStatusDetail("Last run failed: " + history->ResultText, recoveryHint),
			    "warning",
			    recoveryAction.OperationId.isEmpty() ? nullptr : CreateActionDependencyActions(recoveryAction.OperationId, recoveryAction.Label, QString(), QString(), true));
		}
	}

	void LauncherMainWindow::AddStatusRow(QVBoxLayout& layout, const QString& label, const QString& status, const QString& detail, const QString& state, QWidget* accessory)
	{
		QFrame* row = new QFrame(this);
		row->setObjectName("StatusRow");
		QHBoxLayout* rowLayout = new QHBoxLayout(row);
		rowLayout->setContentsMargins(0, 0, 0, 0);
		rowLayout->setSpacing(kSpaceMedium);

		QVBoxLayout* textLayout = new QVBoxLayout();
		textLayout->setContentsMargins(0, 0, 0, 0);
		textLayout->setSpacing(3);

		QLabel* nameLabel = new QLabel(label, row);
		nameLabel->setObjectName("StatusLabel");
		textLayout->addWidget(nameLabel);

		if (!detail.isEmpty())
		{
			QLabel* detailLabel = new QLabel(detail, row);
			detailLabel->setObjectName("StatusDetail");
			detailLabel->setWordWrap(true);
			textLayout->addWidget(detailLabel);
		}

		rowLayout->addLayout(textLayout, 1);

		QLabel* statusLabel = new QLabel(status, row);
		statusLabel->setObjectName("StatusValue");
		statusLabel->setProperty("State", state);
		statusLabel->setFixedWidth(kStatusChipColumnWidth);
		statusLabel->setAlignment(Qt::AlignCenter);
		rowLayout->addWidget(statusLabel, 0, Qt::AlignRight | Qt::AlignTop);

		QWidget* actionCell = new QWidget(row);
		actionCell->setObjectName("StatusActionCell");
		actionCell->setFixedWidth(kStatusActionColumnWidth);
		QHBoxLayout* actionCellLayout = new QHBoxLayout(actionCell);
		actionCellLayout->setContentsMargins(0, 0, 0, 0);
		actionCellLayout->setSpacing(0);
		actionCellLayout->setAlignment(Qt::AlignCenter | Qt::AlignTop);
		if (accessory != nullptr)
		{
			accessory->setParent(actionCell);
			actionCellLayout->addWidget(accessory, 0, Qt::AlignCenter | Qt::AlignTop);
		}
		rowLayout->addWidget(actionCell, 0, Qt::AlignRight | Qt::AlignTop);

		layout.addWidget(row);
	}

	std::filesystem::path LauncherMainWindow::FindLauncherVisualAsset(const QString& fileName) const
	{
		if (fileName.isEmpty())
		{
			return {};
		}

		const std::string assetName = fileName.toStdString();
		const std::array<std::filesystem::path, 4> candidates = {
		    m_repositoryRoot / "Tools" / "Launcher" / "SparkleLauncher" / "Assets" / "Visuals" / assetName,
		    GetArtifactDirectory(m_repositoryRoot) / "dev" / "launcher" / "Visuals" / assetName,
		    GetArtifactDirectory(m_repositoryRoot) / "diagnostics" / "launcher-visual-assets" / assetName,
		    GetArtifactDirectory(m_repositoryRoot) / "diagnostics" / "launcher-visual-assets" / (std::filesystem::path(assetName).stem().string() + ".png")};
		for (const std::filesystem::path& candidate : candidates)
		{
			std::error_code errorCode;
			if (std::filesystem::is_regular_file(candidate, errorCode) && !errorCode)
			{
				return candidate;
			}
		}

		return {};
	}

	QLabel* LauncherMainWindow::CreateVisualArtworkLabel(const QString& fileName, const QString& objectName, const QSize& minimumSize)
	{
		const std::filesystem::path artworkPath = FindLauncherVisualAsset(fileName);
		if (artworkPath.empty())
		{
			return nullptr;
		}

		QPixmap pixmap(QString::fromStdString(artworkPath.string()));
		if (pixmap.isNull())
		{
			return nullptr;
		}

		QLabel* artwork = new QLabel(this);
		artwork->setObjectName(objectName);
		artwork->setPixmap(pixmap);
		artwork->setScaledContents(true);
		artwork->setMinimumSize(minimumSize);
		artwork->setAccessibleName(QStringLiteral("Visual artwork: %1").arg(fileName));
		return artwork;
	}

	QFrame* LauncherMainWindow::CreateHomeHeroCard(
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* primaryAction,
	    QWidget* secondaryAction,
	    const QString& artworkFileName)
	{
		QFrame* card = new QFrame(this);
		card->setObjectName("CommandHeroCard");
		card->setProperty("State", state);
		card->setMinimumHeight(220);
		QHBoxLayout* shellLayout = new QHBoxLayout(card);
		shellLayout->setContentsMargins(0, 0, 0, 0);
		shellLayout->setSpacing(0);

		QWidget* copyPane = new QWidget(card);
		copyPane->setObjectName("CommandHeroCopyPane");
		QVBoxLayout* layout = new QVBoxLayout(copyPane);
		layout->setContentsMargins(22, 18, 22, 18);
		layout->setSpacing(12);

		QHBoxLayout* titleRow = new QHBoxLayout();
		titleRow->setContentsMargins(0, 0, 0, 0);
		titleRow->setSpacing(kSpaceMedium);
		QLabel* title = new QLabel(status, card);
		title->setObjectName("CommandHeroTitle");
		titleRow->addWidget(title, 1);
		QLabel* chip = new QLabel(state == "ok" ? "Ready" : (state == "warning" ? "Needs action" : "Source mode"), card);
		chip->setObjectName("CommandHeroChip");
		chip->setProperty("State", state);
		titleRow->addWidget(chip, 0, Qt::AlignRight | Qt::AlignTop);
		layout->addLayout(titleRow);

		QLabel* body = new QLabel(detail, card);
		body->setObjectName("CommandHeroText");
		body->setWordWrap(true);
		layout->addWidget(body);

		QHBoxLayout* actionRow = new QHBoxLayout();
		actionRow->setContentsMargins(0, 2, 0, 0);
		actionRow->setSpacing(kSpaceSmall);
		if (primaryAction != nullptr)
		{
			primaryAction->setParent(card);
			actionRow->addWidget(primaryAction, 0, Qt::AlignLeft);
		}
		if (secondaryAction != nullptr)
		{
			secondaryAction->setParent(card);
			actionRow->addWidget(secondaryAction, 0, Qt::AlignLeft);
		}
		actionRow->addStretch(1);
		layout->addLayout(actionRow);

		shellLayout->addWidget(copyPane, 1);

		if (QLabel* artwork = CreateVisualArtworkLabel(artworkFileName, "CommandHeroArtwork", QSize(420, 220)))
		{
			artwork->setParent(card);
			shellLayout->addWidget(artwork, 2);
		}
		return card;
	}

	QFrame* LauncherMainWindow::CreateHomeCapabilityCard(
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QString& artworkFileName)
	{
		QFrame* card = new QFrame(this);
		card->setObjectName("CommandCapabilityCard");
		card->setProperty("State", state);
		card->setProperty("TileRole", tileRole);
		const bool hasArtwork = !FindLauncherVisualAsset(artworkFileName).empty();
		card->setMinimumHeight(tileRole == "library" ? (hasArtwork ? 232 : 178) : (hasArtwork ? 190 : 148));
		QVBoxLayout* layout = new QVBoxLayout(card);
		layout->setContentsMargins(tileRole == "library" ? 18 : 16, hasArtwork ? 12 : (tileRole == "library" ? 16 : 14), tileRole == "library" ? 18 : 16, tileRole == "library" ? 16 : 14);
		layout->setSpacing(tileRole == "library" ? 12 : 10);

		if (QLabel* artwork = CreateVisualArtworkLabel(artworkFileName, "CommandCardArtwork", QSize(260, tileRole == "library" ? 112 : 86)))
		{
			artwork->setParent(card);
			layout->addWidget(artwork);
		}

		QHBoxLayout* titleRow = new QHBoxLayout();
		titleRow->setContentsMargins(0, 0, 0, 0);
		titleRow->setSpacing(kSpaceSmall);
		QLabel* titleLabel = new QLabel(title, card);
		titleLabel->setObjectName("CommandCardTitle");
		titleRow->addWidget(titleLabel, 1);
		QLabel* statusLabel = new QLabel(status, card);
		statusLabel->setObjectName("CommandCardChip");
		statusLabel->setProperty("State", state);
		titleRow->addWidget(statusLabel, 0, Qt::AlignRight | Qt::AlignTop);
		layout->addLayout(titleRow);

		QLabel* detailLabel = new QLabel(detail, card);
		detailLabel->setObjectName("CommandCardText");
		detailLabel->setWordWrap(true);
		layout->addWidget(detailLabel, 1);

		if (action != nullptr)
		{
			action->setParent(card);
			layout->addWidget(action, 0, Qt::AlignLeft);
		}
		return card;
	}

	QPushButton* LauncherMainWindow::CreateCommandActionButton(const QString& operationId, const QString& label, bool primary, bool runImmediately)
	{
		QPushButton* button = new QPushButton(label, this);
		button->setObjectName(primary ? "CommandPrimaryButton" : "CommandSecondaryButton");
		button->setMinimumHeight(primary ? 34 : 30);
		button->setAccessibleName(label);
		button->setToolTip(runImmediately ? "Run this workflow now." : "Open this workflow.");
		RegisterFocusable(button);
		connect(button, &QPushButton::clicked, this, [this, operationId, runImmediately]() {
			SetSelectedOperation(operationId);
			if (runImmediately)
			{
				RunSelectedOperation();
			}
		});
		return button;
	}

	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
		const BuildWorkspaceOperationRequest workspaceRequest = MakeWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
		const BuildWorkspaceOperationPlan workspacePlan = PlanBuildWorkspaceOperation("workspace.generate-solution", workspaceRequest);
		const BuildWorkspaceOperationPlan packagePlan = PlanBuildWorkspaceOperation("package.release", workspaceRequest);
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const std::filesystem::path releaseRoot = m_repositoryRoot / "dist" / "releases";
		const bool sourceRoot = PathExists(m_repositoryRoot / "CMakeLists.txt");
		const bool packageRoot = PathExists(m_repositoryRoot / "SparkleLauncher.exe") && DirectoryHasEntries(m_repositoryRoot / "manifests");

		const auto planLaunch = [this](const QString& operationId) {
			LauncherOperationRequest request = BuildOperationRequest(operationId);
			LaunchOperationRequest launchRequest;
			launchRequest.RepositoryRoot = request.RepositoryRoot;
			launchRequest.OperationId = operationId.toStdString();
			launchRequest.ProjectId = request.ProjectId.toStdString();
			launchRequest.EditorProfile = request.EditorProfile.toStdString();
			launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
			launchRequest.Target = request.LaunchTarget.toStdString();
			launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
			launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
			launchRequest.VSync = request.LaunchVSync.toStdString();
			launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
			launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
			for (const QString& argument : QProcess::splitCommand(request.LaunchCommandLineArguments))
			{
				if (!argument.isEmpty())
				{
					launchRequest.CustomArguments.push_back(argument.toStdString());
				}
			}
			for (const QString& part : request.LaunchCVars.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
			{
				const QString trimmed = part.trimmed();
				if (!trimmed.isEmpty())
				{
					launchRequest.CustomCVars.push_back(trimmed.toStdString());
				}
			}
			launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
			launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
			launchRequest.SmokeTrace = request.SmokeTrace;
			launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;
			return PlanLaunchOperation(operationId.toStdString(), launchRequest);
		};

		const LaunchOperationPlan editorPlan = planLaunch("project.open.editor");
		const LaunchOperationPlan runtimePlan = planLaunch("project.open.runtime");
		const bool editorExecutableMissing = ReadinessContains(editorPlan.ReadinessMessages, "Executable is missing");
		const bool runtimeExecutableMissing = ReadinessContains(runtimePlan.ReadinessMessages, "Executable is missing");
		const bool cookedMeshesMissing = ReadinessContains(editorPlan.ReadinessMessages, "Cooked scene assets are missing") ||
		    ReadinessContains(runtimePlan.ReadinessMessages, "Cooked scene assets are missing");
		const bool cookedTexturesMissing = ReadinessContains(editorPlan.ReadinessMessages, "Cooked textures are missing") ||
		    ReadinessContains(runtimePlan.ReadinessMessages, "Cooked textures are missing");
		const bool cookedShadersMissing = ReadinessContains(editorPlan.ReadinessMessages, "Cooked shaders are missing") ||
		    ReadinessContains(runtimePlan.ReadinessMessages, "Cooked shaders are missing");
		const int missingCookDomains = static_cast<int>(cookedMeshesMissing) + static_cast<int>(cookedTexturesMissing) + static_cast<int>(cookedShadersMissing);
		HomeNextAction primaryAction;
		if (m_projectModel.SelectedProjectId().isEmpty())
		{
			primaryAction = {"", "Check Project Root", "No project marker is selected. Confirm this root contains Projects/<Project> markers before launching, building, cooking, or validating a project.", true};
		}
		else if (editorPlan.CanRun)
		{
			primaryAction = {"project.open.editor", "Open Editor", "The selected project editor is ready. Launch first; rebuilds and recooks remain optional refresh paths.", false};
		}
		else if (runtimePlan.CanRun)
		{
			primaryAction = {"project.open.runtime", "Open Runtime", "The selected project runtime is ready. Launch first; rebuilds and recooks remain optional refresh paths.", false};
		}
		else if (!workspacePlan.Toolchain.RequiredToolsAvailable)
		{
			primaryAction = {"toolchain.check", "Verify Host Environment", "Installed host tools are blocking source rebuild, cook, or package workflows.", true};
		}
		else if (!workspacePlan.Freshness.Current)
		{
			primaryAction = {"workspace.generate-solution", "Generate Build Files", "Generated CMake and IDE files are stale for the selected toolchain.", true};
		}
		else if (editorExecutableMissing)
		{
			primaryAction = {"project.build.editor", "Build Editor", "The selected project editor executable is missing from local artifacts.", true};
		}
		else if (runtimeExecutableMissing)
		{
			primaryAction = {"project.build.runtime", "Build Runtime", "The selected project runtime executable is missing from local artifacts.", true};
		}
		else if (missingCookDomains > 1)
		{
			primaryAction = {"cook.project", "Cook Missing Content", "Multiple cooked content domains are missing; run the project cook from the Cook workflow.", true};
		}
		else if (cookedMeshesMissing)
		{
			primaryAction = {"cook.assets", "Cook Scenes And Meshes", "Scene and mesh cooked outputs are missing for the selected project.", true};
		}
		else if (cookedTexturesMissing)
		{
			primaryAction = {"cook.textures", "Cook Textures", "Cooked texture outputs are missing for the selected project.", true};
		}
		else if (cookedShadersMissing)
		{
			primaryAction = {"cook.shaders", "Cook Shaders", "Cooked shader outputs are missing for the selected project.", true};
		}
		else
		{
			primaryAction = {"workspace.setup", "Sync Source Tiers", "Source tiers can unlock more build and cook capability without changing installed host tools.", true};
		}

		HomeNextAction secondaryAction = {"project.run.smoke", "Run Smoke Test", "Test the selected editor or runtime once launch outputs are ready.", true};
		if (!workspacePlan.Freshness.Current && primaryAction.OperationId != "workspace.generate-solution")
		{
			secondaryAction = {"workspace.generate-solution", "Generate Build Files", "Refresh CMake and IDE files before local development work.", true};
		}
		else if (missingCookDomains > 0 && !primaryAction.OperationId.startsWith("cook."))
		{
			secondaryAction = {"cook.project", "Cook Missing Content", "Refresh generated project content when launching from source artifacts.", true};
		}
		else if (!packagePlan.CanRun)
		{
			secondaryAction = {"package.release", "Inspect Package Assembly", FirstBlockingReadinessMessage(packagePlan), true};
		}

		int enabledDependencyCount = 0;
		int readyDependencyCount = 0;
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			if (!group.Enabled)
			{
				continue;
			}
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				++enabledDependencyCount;
				if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
				{
					++readyDependencyCount;
				}
			}
		}

		const bool architectureDoc = PathExists(m_repositoryRoot / "docs" / "plans" / "build-artifacts-release-architecture-roadmap.md");
		const bool uxDoc = PathExists(m_repositoryRoot / "docs" / "plans" / "launcher-principal-ux-concept.md");
		const bool dependencyDoc = PathExists(m_repositoryRoot / "docs" / "dependency-capability-tiers.md");
		const std::filesystem::path validationReportPath = m_repositoryRoot / "docs" / "plans" / "build-artifacts-phase6-final-validation-report.md";
		const bool validationReport = PathExists(validationReportPath);
		int storedFailureCount = 0;
		for (auto it = m_actionHistory.cbegin(); it != m_actionHistory.cend(); ++it)
		{
			if (it.value().ExitCode != 0)
			{
				++storedFailureCount;
			}
		}
		const auto createOpenButton = [this](const QString& label, const std::filesystem::path& path) {
			QPushButton* button = new QPushButton(label, this);
			button->setObjectName("CommandSecondaryButton");
			button->setMinimumHeight(30);
			button->setToolTip("Open the referenced file or folder.");
			button->setAccessibleName(label);
			RegisterFocusable(button);
			connect(button, &QPushButton::clicked, this, [this, path]() {
				OpenLocalPath(path);
			});
			return button;
		};
		const auto addHomeSection = [&layout](const QString& title) {
			QLabel* section = new QLabel(title);
			section->setObjectName("CommandSectionTitle");
			section->setAccessibleName(title);
			layout.addWidget(section);
		};

		QFrame* identity = new QFrame(this);
		identity->setObjectName("CommandIdentityBar");
		QHBoxLayout* identityLayout = new QHBoxLayout(identity);
		identityLayout->setContentsMargins(0, 0, 0, 0);
		identityLayout->setSpacing(kSpaceMedium);
		QVBoxLayout* titleLayout = new QVBoxLayout();
		titleLayout->setContentsMargins(0, 0, 0, 0);
		titleLayout->setSpacing(2);
		QLabel* productTitle = new QLabel("Sparkle Engine", identity);
		productTitle->setObjectName("CommandProductTitle");
		titleLayout->addWidget(productTitle);
		QLabel* productSubtitle = new QLabel("Showcase project command center", identity);
		productSubtitle->setObjectName("CommandProductSubtitle");
		titleLayout->addWidget(productSubtitle);
		identityLayout->addLayout(titleLayout, 1);
		layout.addWidget(identity);

		const bool launchReady = editorPlan.CanRun || runtimePlan.CanRun;
		const QString heroState = launchReady ? "ok" : "warning";
		const QString launchProvenance = packageRoot ? "bundled package components" : "local source artifacts";
		const QString heroTitle = launchReady ? (editorPlan.CanRun ? "Open Showcase Editor" : "Open Showcase Runtime") :
		                          (!workspacePlan.Freshness.Current && !editorExecutableMissing && !runtimeExecutableMissing ? "Generate Build Files" :
		                                                                                                         primaryAction.Label);
		const QString heroDetail = launchReady ?
		    QStringLiteral("Launch from %1. Rebuild and recook remain optional refresh paths.").arg(launchProvenance) :
		    primaryAction.Detail;
		layout.addWidget(CreateHomeHeroCard(
		    heroTitle,
		    heroDetail,
		    heroState,
		    primaryAction.OperationId.isEmpty() ? nullptr : CreateCommandActionButton(primaryAction.OperationId, primaryAction.Label, true, !primaryAction.NavigateOnly),
		    secondaryAction.OperationId.isEmpty() ? nullptr : CreateCommandActionButton(secondaryAction.OperationId, secondaryAction.Label, false),
		    "showcase-hero.png"));

		addHomeSection("Library");
		QGridLayout* libraryGrid = new QGridLayout();
		libraryGrid->setContentsMargins(0, 0, 0, 4);
		libraryGrid->setHorizontalSpacing(16);
		libraryGrid->setVerticalSpacing(16);

		const QString editorStatus = editorPlan.CanRun ? "Ready" : (editorExecutableMissing ? "Missing" : "Blocked");
		const QString editorDetail = editorPlan.CanRun ?
		    QStringLiteral("Launch the Showcase editor from %1.").arg(launchProvenance) :
		    "Editor output is not ready yet. Build the editor target to unlock this launch path.";
		libraryGrid->addWidget(CreateHomeCapabilityCard(
		                           "Showcase Editor",
		                           editorStatus,
		                           editorDetail,
		                           editorPlan.CanRun ? "ok" : "warning",
		                           CreateCommandActionButton(editorPlan.CanRun ? "project.open.editor" : "project.build.editor", editorPlan.CanRun ? "Open Editor" : "Build Editor", false, editorPlan.CanRun),
		                           "library",
		                           "showcase-editor.png"),
		    0,
		    0);
		const QString runtimeStatus = runtimePlan.CanRun ? "Ready" : (runtimeExecutableMissing ? "Missing" : "Blocked");
		const QString runtimeDetail = runtimePlan.CanRun ?
		    QStringLiteral("Run the Showcase runtime from %1.").arg(launchProvenance) :
		    "Runtime output is not ready yet. Build the runtime target to unlock the standalone path.";
		libraryGrid->addWidget(CreateHomeCapabilityCard(
		                           "Showcase Runtime",
		                           runtimeStatus,
		                           runtimeDetail,
		                           runtimePlan.CanRun ? "ok" : "warning",
		                           CreateCommandActionButton(runtimePlan.CanRun ? "project.open.runtime" : "project.build.runtime", runtimePlan.CanRun ? "Open Runtime" : "Build Runtime", false, runtimePlan.CanRun),
		                           "library",
		                           "showcase-runtime.png"),
		    0,
		    1);
		layout.addLayout(libraryGrid);

		addHomeSection("Discover");
		QGridLayout* discoverGrid = new QGridLayout();
		discoverGrid->setContentsMargins(0, 0, 0, 0);
		discoverGrid->setHorizontalSpacing(16);
		discoverGrid->setVerticalSpacing(16);

		const bool packagePresent = DirectoryHasEntries(releaseRoot);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Architecture",
		                            architectureDoc ? "Available" : "Pending",
		                            "Product boundaries, artifact layout, packaging model, and launcher workflow intent.",
		                            architectureDoc ? "ok" : "warning",
		                            architectureDoc ? createOpenButton("Open Architecture", m_repositoryRoot / "docs" / "plans" / "build-artifacts-release-architecture-roadmap.md") : nullptr,
		                            "sparkle-architecture.png"),
		    0,
		    0);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Dependency Tiers",
		                            dependencyDoc ? "Available" : "Pending",
		                            readyDependencyCount == enabledDependencyCount ? "Enabled source tiers are cached; optional tiers unlock more build and cook capability." :
		                                                                          "Sync source tiers only when you need the extra local build or cook capability.",
		                            dependencyDoc ? "ok" : "warning",
		                            dependencyDoc ? createOpenButton("Open Tiers", m_repositoryRoot / "docs" / "dependency-capability-tiers.md") : CreateCommandActionButton("workspace.setup", "Sync Source Tiers", false),
		                            "sparkle-source-tiers.png"),
		    0,
		    1);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Validation",
		                            validationReport ? "Available" : (storedFailureCount == 0 ? "No active issue" : "Review activity"),
		                            validationReport ? "Open the latest final validation report." :
		                                               "Run smoke tests or open Activity when you want runtime confidence.",
		                            validationReport || storedFailureCount == 0 ? "ok" : "warning",
		                            validationReport ? createOpenButton("Open Report", validationReportPath) : CreateCommandActionButton("project.run.smoke", "Run Smoke Test", false),
		                            "sparkle-validation.png"),
		    0,
		    2);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Package",
		                            packagePresent ? "Present" : (packagePlan.CanRun ? "Ready" : "Blocked"),
		                            packagePresent ? "Open assembled release folders." : "Assemble a release package from artifacts; publishing remains separate.",
		                            packagePresent || packagePlan.CanRun ? "ok" : "warning",
		                            packagePresent ? createOpenButton("Open Packages", releaseRoot) : CreateCommandActionButton("package.release", "Assemble", false),
		                            "sparkle-package.png"),
		    1,
		    0);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Content",
		                            missingCookDomains == 0 ? "Ready" : QStringLiteral("%1 missing").arg(missingCookDomains),
		                            missingCookDomains == 0 ? "Cooked content is ready for launch workflows." : "Cook only the missing generated content when local artifacts need it.",
		                            missingCookDomains == 0 ? "ok" : "warning",
		                            CreateCommandActionButton("cook.project", missingCookDomains == 0 ? "Cook All" : "Cook Missing", false),
		                            "showcase-content.png"),
		    1,
		    1);
		discoverGrid->addWidget(CreateHomeCapabilityCard(
		                            "Tools",
		                            uxDoc ? "Available" : "Pending",
		                            "Open UX notes or continue into Sync, Build, Cook, Test, Package, and System workflows from the rail.",
		                            uxDoc ? "ok" : "warning",
		                            uxDoc ? createOpenButton("Open UX Notes", m_repositoryRoot / "docs" / "plans" / "launcher-principal-ux-concept.md") : nullptr,
		                            "sparkle-tools.png"),
		    1,
		    2);
		layout.addLayout(discoverGrid);
	}

	void LauncherMainWindow::AddSystemOverviewPage(QVBoxLayout& layout)
	{
		BuildWorkspaceOperationRequest request = MakeWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation("workspace.generate-solution", request);
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const LauncherStatePaths statePaths = GetLauncherStatePaths(m_repositoryRoot);
		int enabledDependencyCount = 0;
		int readyDependencyCount = 0;
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			if (!group.Enabled)
			{
				continue;
			}
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				++enabledDependencyCount;
				if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
				{
					++readyDependencyCount;
				}
			}
		}

		QVBoxLayout* statsLayout = AddOptionGroup(layout, "Statistics", "Current workspace and machine state, kept compact for daily production checks.");
		AddStatusRow(*statsLayout, "Project", m_projectModel.SelectedProjectId().isEmpty() ? "Not selected" : m_projectModel.SelectedProjectId(), "Selected project used by launch, build, cook, and test workflows.", m_projectModel.SelectedProjectId().isEmpty() ? "warning" : "ok");
		AddStatusRow(*statsLayout, "Root mode", PathExists(m_repositoryRoot / "SparkleLauncher.exe") ? "Package" : "Source checkout", "Package mode uses bundled components first; source checkout uses local artifacts and generated workspace state.", "neutral");
		AddStatusRow(*statsLayout, "Toolchain", plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed", BuildGeneratorSummary(plan.Toolchain), plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad", CreateActionDependencyActions("toolchain.check", "Verify Host Environment", QString(), QString(), true));
		AddStatusRow(*statsLayout, "Build files", plan.Freshness.Current ? "Current" : "Needs refresh", QString::fromStdString(plan.Freshness.Summary), plan.Freshness.Current ? "ok" : "warning", CreateActionDependencyActions("workspace.generate-solution", "Generate Build Files", "build-tree", "Clean Build Files", true));
		AddStatusRow(*statsLayout, "Source tiers", QStringLiteral("%1 / %2 cached").arg(readyDependencyCount).arg(enabledDependencyCount), "Enabled source tiers unlock optional local build and cook capabilities.", readyDependencyCount == enabledDependencyCount ? "ok" : "warning", CreateActionDependencyActions("workspace.setup", "Sync Source Tiers", "deps", "Clean Source Dependency Cache", true));

		QVBoxLayout* artifactLayout = AddDetailsGroup(layout, "Locations", "Declared workspace roots. Paths are intentionally kept in System rather than Home.", false);
		AddStatusRow(*artifactLayout, "Build root", "Generated", ToDisplayPath(m_repositoryRoot, GetBuildDirectory(m_repositoryRoot)), "neutral");
		AddStatusRow(*artifactLayout, "Artifacts root", "Generated", ToDisplayPath(m_repositoryRoot, GetArtifactDirectory(m_repositoryRoot)), "neutral");
		AddStatusRow(*artifactLayout, "Dist root", "Package output", ToDisplayPath(m_repositoryRoot, m_repositoryRoot / "dist"), "neutral");
		AddStatusRow(*artifactLayout, "Launcher logs", "Available", ToDisplayPath(m_repositoryRoot, statePaths.LogsDirectory), "neutral");

		AddSourceTierCards(layout, "Source Tiers", "Capability workload cards. Sync only the tiers needed for the local work you intend to do.", false);
	}

	void LauncherMainWindow::AddBuildEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : m_projectModel.SelectedProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RuntimeProfile = m_settings.RuntimeProfile().toStdString();
		request.PreferredIde = SelectedWorkspaceIde(m_settings);
		request.ForceConfigure = m_settings.ForceConfigure();

		const QString workspacePlanOperationId =
		    operationId.startsWith("cook.") && operationId != "cook.tools.prepare" ? "cook.tools.prepare" : operationId;
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(workspacePlanOperationId.toStdString(), request);
		const QString workspaceIdeName = SelectedWorkspaceIdeName(m_settings);
		const bool isToolchainCheck = operationId == "toolchain.check";
		const bool isSetupWorkflow = operationId == "workspace.setup" || operationId == "workspace.generate-solution" || operationId == "workspace.open-solution";
		const bool isBuildWorkflow = operationId == "workspace.build-all" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare" || operationId == "launcher.build.self";
		const bool isCookWorkflow = operationId.startsWith("cook.") && operationId != "cook.tools.prepare";
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const bool dependencyCacheReady = DirectoryHasEntries(dependencyCachePath);
		const auto addRelevantDependencyGroups = [this, &dependencyCachePath, &operationId](QVBoxLayout& targetLayout) {
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				if (!OperationUsesDependencyGroup(operationId, group))
				{
					continue;
				}

				const int readyCount = CountReadyDependencies(group, dependencyCachePath);
				AddStatusRow(
				    targetLayout,
				    group.Label,
				    DependencyGroupStatusText(group, readyCount),
				    FormatDependencyGroupDetail(group, dependencyCachePath, readyCount),
				    DependencyGroupStatusState(group, readyCount),
				    group.Enabled ? CreateActionDependencyActions("workspace.setup", "Sync Source Tiers", "deps", "Clean Source Dependency Cache") :
				                    CreateDisabledSourceTierActions(group));
			}
		};
		const auto addHostDependencyStatus = [this, &plan, &request, &workspaceIdeName](QVBoxLayout& targetLayout, const QString& detailText) {
			QVBoxLayout* hostLayout = AddDetailsGroup(
			    targetLayout,
			    "Host Rebuild Dependency Details",
			    detailText,
			    !plan.Toolchain.RequiredToolsAvailable);
			AddStatusRow(
			    *hostLayout,
			    "Dependency set",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed",
			    BuildGeneratorSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			if (plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *hostLayout,
				    "Tool details",
				    "Available in host audit",
				    "Open Verify Host Environment for the full installed-tool inventory.",
				    "neutral",
				    CreateActionDependencyActions("toolchain.check", "Verify Host Environment", QString(), QString(), true));
				return;
			}
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				QString detail = QString::fromStdString(item.Detail);
				const QString path = FormatStatusPath(item.Path);
				if (!path.isEmpty())
				{
					detail = CombineStatusDetail(detail, path);
				}
				AddStatusRow(
				    *hostLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    detail,
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *hostLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
		};
		if (isToolchainCheck)
		{
			QVBoxLayout* toolchainLayout = AddDetailsGroup(
			    layout,
			    plan.Toolchain.RequiredToolsAvailable ? "Action Dependencies - Ready" : "Action Dependencies - Needs action",
			    "Authoritative machine audit for local rebuilds, workspace generation, cook tooling, and IDE integration.",
			    false);
			AddStatusRow(*toolchainLayout, "Dependency set", plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Action needed", BuildGeneratorSummary(plan.Toolchain), plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad");
			AddStatusRow(
			    *toolchainLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : "Rider executable is available.") :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : "Visual Studio workspace discovery is available."),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
			QVBoxLayout* hostDetailsLayout = AddDetailsGroup(
			    layout,
			    "Host Tool Details",
			    "Full installed-tool inventory and raw tool paths for diagnostics.",
			    !plan.Toolchain.RequiredToolsAvailable);
			for (const ToolchainItemStatus& item : plan.Toolchain.Items)
			{
				QString detail = QString::fromStdString(item.Detail);
				const QString path = FormatStatusPath(item.Path);
				if (!path.isEmpty())
				{
					detail = CombineStatusDetail(detail, path);
				}
				AddStatusRow(
				    *hostDetailsLayout,
				    QString::fromStdString(item.DisplayName) + (item.Required ? "" : " (optional)"),
				    ToolchainStatusText(item.State, item.Required),
				    detail,
				    ToolchainStatusState(item.State, item.Required));
			}
			AddStatusRow(
			    *hostDetailsLayout,
			    "Selected IDE",
			    workspaceIdeName,
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "Rider executable was not found." : QString::fromStdString(plan.Toolchain.RiderPath.string())) :
			                                                 (plan.Toolchain.VswherePath.empty() ? "Visual Studio discovery is not ready." : QString::fromStdString(plan.Freshness.SolutionPath.string())),
			    request.PreferredIde == WorkspaceIde::Rider ? (plan.Toolchain.RiderPath.empty() ? "warning" : "ok") : (plan.Toolchain.VswherePath.empty() ? "warning" : "ok"));
			return;
		}

		if (isSetupWorkflow)
		{
			const QString buildFilesLabel = operationId == "workspace.generate-solution" ? "Generated build files" :
			                               operationId == "workspace.open-solution" ? "IDE build files" :
			                                                                        "Configured build files";
			const bool isGenerateSolutionWorkflow = operationId == "workspace.generate-solution";
			const bool isSourceSyncWorkflow = operationId == "workspace.setup";
			const bool isOpenIdeWorkflow = operationId == "workspace.open-solution";
			const QString setupGroupTitle = isGenerateSolutionWorkflow ? "Build File Generation" :
			                                isOpenIdeWorkflow ? "IDE Launch Readiness" :
			                                                    "Source Tier Sync";
			const QString setupGroupDetail = isGenerateSolutionWorkflow ?
			                                     "Refresh generated CMake and IDE build-system files to match the selected generator, platform, toolset, and Qt kit." :
			                                 isOpenIdeWorkflow ?
			                                     "Open the selected IDE only after generated build files match the current toolchain selection." :
			                                     "Sync enabled source-tier capability groups and configure state. This does not install Visual Studio, Qt, CMake, Git, the Windows SDK, or other host prerequisites.";
			const QString buildFilesDetail = CombineStatusDetail(
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    request.PreferredIde == WorkspaceIde::Rider ? QString("Repository root") : ToDisplayPath(m_repositoryRoot, plan.Freshness.SolutionPath));
			const QString cacheStatus = dependencyCacheReady ? "Ready" : "Will be created";
			const QString cacheDetail = dependencyCacheReady ?
			                               QString("Source dependency cache is available.") :
			                               QString("Source dependency cache will be populated when Sync Source Tiers runs.");
			const bool setupNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || (isSourceSyncWorkflow && !dependencyCacheReady) || !plan.Freshness.Current;
			QVBoxLayout* workspaceLayout = AddDetailsGroup(
			    layout,
			    setupNeedsAttention ? setupGroupTitle + " - Needs action" : setupGroupTitle + " - Ready",
			    setupGroupDetail,
			    setupNeedsAttention);
			AddStatusRow(
			    *workspaceLayout,
			    buildFilesLabel,
			    isGenerateSolutionWorkflow ? (plan.Freshness.Current ? "Current" : "Will be refreshed") :
			                                (plan.Freshness.Current ? "Ready" : "Needs refresh"),
			    buildFilesDetail,
			    plan.Freshness.Current ? "ok" : "warning",
			    isGenerateSolutionWorkflow ? nullptr :
			                                 CreateActionDependencyActions("workspace.generate-solution", "Generate Build Files", "build-tree", "Clean Build Files"));
			if (isSourceSyncWorkflow)
			{
				AddStatusRow(
				    *workspaceLayout,
				    "Local source dependency cache",
				    cacheStatus,
				    CombineStatusDetail(cacheDetail, FormatTrackedDependencySummary(dependencyCachePath)),
				    dependencyCacheReady ? "ok" : "warning",
				    CreateActionDependencyActions("workspace.setup", "Sync Source Tiers", "deps", "Clean Source Dependency Cache"));
			}
			if (!plan.Toolchain.RequiredToolsAvailable)
			{
				AddStatusRow(
				    *workspaceLayout,
				    "Required tools",
				    "Blocked",
				    RequiredToolProblemSummary(plan.Toolchain),
				    "bad",
				    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			}
			if (operationId == "workspace.setup")
			{
				AddSourceTierCards(
				    layout,
				    "Source Tier Workloads",
				    "Capability cards show what each tier unlocks. Individual dependency rows stay in details so Sync Source Tiers does not become a dependency log by default.",
				    true);
			}
			return;
		}

		if (isBuildWorkflow)
		{
			const bool buildNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current;
			QVBoxLayout* buildLayout = AddDetailsGroup(
			    layout,
			    buildNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Requirements this build workflow depends on before a local rebuild can run successfully.",
			    false);
			AddStatusRow(
			    *buildLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			AddStatusRow(
			    *buildLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    CreateActionDependencyActions("workspace.generate-solution", "Generate Build Files", "build-tree", "Clean Build Files"));
			addRelevantDependencyGroups(*buildLayout);
			addHostDependencyStatus(
			    layout,
			    operationId == "launcher.build.self" ?
			        "Even if this launcher came from a ready-to-run package, rebuilding it locally still requires Visual Studio/MSVC, a Qt 6 MSVC kit, CMake, Git, and the Windows SDK." :
			        "Prebuilt package binaries do not remove the local host dependencies needed to rebuild this workspace.");
			return;
		}

		if (isCookWorkflow)
		{
			const bool cookNeedsAttention = !plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current;
			QVBoxLayout* cookLayout = AddDetailsGroup(
			    layout,
			    cookNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Requirements this cook workflow depends on before local recook operations can run successfully.",
			    false);
			AddStatusRow(
			    *cookLayout,
			    "Required tools",
			    plan.Toolchain.RequiredToolsAvailable ? "Ready" : "Blocked",
			    plan.Toolchain.RequiredToolsAvailable ? BuildGeneratorSummary(plan.Toolchain) : RequiredToolProblemSummary(plan.Toolchain),
			    plan.Toolchain.RequiredToolsAvailable ? "ok" : "bad",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			AddStatusRow(
			    *cookLayout,
			    "Build files",
			    plan.Freshness.Current ? "Ready" : "Needs refresh",
			    CombineStatusDetail(QString::fromStdString(plan.Freshness.Summary), BuildFilesRecoveryHint(plan.Freshness)),
			    plan.Freshness.Current ? "ok" : "warning",
			    CreateActionDependencyActions("workspace.generate-solution", "Generate Build Files", "build-tree", "Clean Build Files"));
			addRelevantDependencyGroups(*cookLayout);
			addHostDependencyStatus(
			    layout,
			    "Cook workflows may ship ready-to-use outputs, but rebuilding or recooking them locally still requires the same host build dependencies and any enabled optional dependency groups.");
		}
	}

	void LauncherMainWindow::AddLaunchEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		LauncherOperationRequest request = BuildOperationRequest(operationId);
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.OperationId = operationId.toStdString();
		launchRequest.ProjectId = request.ProjectId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.Target = request.LaunchTarget.toStdString();
		launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
		for (const QString& argument : QProcess::splitCommand(request.LaunchCommandLineArguments))
		{
			if (!argument.isEmpty())
			{
				launchRequest.CustomArguments.push_back(argument.toStdString());
			}
		}
		for (const QString& part : request.LaunchCVars.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				launchRequest.CustomCVars.push_back(trimmed.toStdString());
			}
		}
		launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
		launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
		launchRequest.SmokeTrace = request.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;

		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), launchRequest);
		const bool runtimeTarget = launchRequest.Target == "runtime";
		const auto findReadiness = [&plan](const QString& prefix) {
			for (const std::string& message : plan.ReadinessMessages)
			{
				const QString text = QString::fromStdString(message);
				if (text.startsWith(prefix, Qt::CaseInsensitive))
				{
					return text;
				}
			}
			return QString();
		};

		const QString executableDetail = findReadiness("Executable ");
		const QString projectDetail = findReadiness("Project working directory ");
		const QString cookedMeshesDetail = findReadiness("Cooked scene assets ");
		const QString cookedTexturesDetail = findReadiness("Cooked textures ");
		const QString cookedShadersDetail = findReadiness("Cooked shaders ");

		const bool launchNeedsAttention =
		    executableDetail.contains("missing", Qt::CaseInsensitive) ||
		    projectDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ||
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive);
		QVBoxLayout* launchLayout = AddDetailsGroup(
		    layout,
		    launchNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
		    "Launch workflows use bundled runtime components when packages provide them, then local rebuild outputs when developing from source.",
		    false);
		AddStatusRow(
		    *launchLayout,
		    "Bundled runtime component",
		    "Supported",
		    "Package-root launches use bundled editor/runtime components and cooked assets from dist/; source checkouts use product artifacts under artifacts/dev.",
		    "neutral");
		AddStatusRow(
		    *launchLayout,
		    runtimeTarget ? "Runtime executable" : "Editor executable",
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    executableDetail,
		    executableDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions(runtimeTarget ? "project.build.runtime" : "project.build.editor", runtimeTarget ? "Build Runtime" : "Build Editor"));
		AddStatusRow(
		    *launchLayout,
		    "Project directory",
		    projectDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    projectDetail,
		    projectDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok");
		AddStatusRow(
		    *launchLayout,
		    "Cooked scene assets",
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedMeshesDetail,
		    cookedMeshesDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.assets", "Cook Scenes And Meshes"));
		AddStatusRow(
		    *launchLayout,
		    "Cooked textures",
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedTexturesDetail,
		    cookedTexturesDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.textures", "Cook Textures"));
		AddStatusRow(
		    *launchLayout,
		    "Cooked shaders",
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive) ? "Missing" : "Ready",
		    cookedShadersDetail,
		    cookedShadersDetail.contains("missing", Qt::CaseInsensitive) ? "warning" : "ok",
		    CreateActionDependencyActions("cook.shaders", "Cook Shaders"));
	}

	void LauncherMainWindow::AddLaunchTargetOptions(QVBoxLayout& layout, const QString& title, const QString& detail)
	{
		QVBoxLayout* targetLayout = AddOptionGroup(layout, title, detail);
		AddOptionField(
		    *targetLayout,
		    "Target",
		    CreateValueCombo(
		        {{"Editor", "editor"}, {"Runtime", "runtime"}},
		        m_settings.LaunchTarget(),
		        &LauncherSettings::SetLaunchTarget));
	}

	void LauncherMainWindow::AddLaunchApplicationOptions(QVBoxLayout& layout)
	{
		QVBoxLayout* appOptionsLayout = AddOptionGroup(layout, "Options", "Arguments and runtime CVars passed to the selected process.");
		AddOptionField(
		    *appOptionsLayout,
		    "Graphics backend",
		    CreateValueCombo(
		        {{"D3D12", ""}, {"Vulkan", "vulkan"}},
		        m_settings.LaunchBackend(),
		        &LauncherSettings::SetLaunchBackend));
		AddOptionField(
		    *appOptionsLayout,
		    "VSync",
		    CreateValueCombo(
		        {{"On", ""}, {"Off", "false"}},
		        m_settings.LaunchVSync(),
		        &LauncherSettings::SetLaunchVSync));
		AddOptionField(
		    *appOptionsLayout,
		    "GPU preference",
		    CreateValueCombo(
		        {{"High performance", ""}, {"System default", "false"}},
		        m_settings.LaunchHighPerformanceAdapter(),
		        &LauncherSettings::SetLaunchHighPerformanceAdapter));
		AddOptionField(
		    *appOptionsLayout,
		    "Arguments",
		    CreateBoundLineEdit(
		        m_settings.LaunchCommandLineArguments(),
		        "--flag value \"quoted value\"",
		        "Extra command-line arguments appended after launcher-managed options.",
		        &LauncherSettings::SetLaunchCommandLineArguments));
		AddOptionField(
		    *appOptionsLayout,
		    "CVars",
		    CreateBoundTextEdit(
		        m_settings.LaunchCVars(),
		        "r.SomeCVar=1\nr.OtherCVar=false",
		        "One CVar assignment per line, comma, or semicolon. Each entry is passed as --cvar name=value.",
		        &LauncherSettings::SetLaunchCVars));
	}

	void LauncherMainWindow::AddSmokeValidationOptions(QVBoxLayout& layout)
	{
		QVBoxLayout* smokeOptionsLayout = AddOptionGroup(layout, "Smoke Test Options", "Smoke-test controls for capture length and diagnostic behavior.");
		AddOptionField(
		    *smokeOptionsLayout,
		    "Frame limit",
		    CreateValueCombo(
		        {{"120 frames", ""}, {"60 frames", "60"}, {"300 frames", "300"}, {"600 frames", "600"}},
		        m_settings.SmokeFrameLimit(),
		        &LauncherSettings::SetSmokeFrameLimit));
		AddOptionCheckBox(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Capture trace",
		        "Write smoke trace output.",
		        m_settings.SmokeTrace(),
		        &LauncherSettings::SetSmokeTrace));
		AddOptionCheckBox(
		    *smokeOptionsLayout,
		    CreateBoundCheckBox(
		        "Skip level switching",
		        "Do not switch levels during smoke.",
		        m_settings.SmokeSkipLevelSwitching(),
		        &LauncherSettings::SetSmokeSkipLevelSwitching));
	}

	void LauncherMainWindow::AddMaintenanceEnvironmentStatus(QVBoxLayout& layout, const QString& operationId)
	{
		MaintenanceOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.ProjectId = m_projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : m_projectModel.SelectedProjectId().toStdString();
		request.EditorProfile = m_settings.EditorProfile().toStdString();
		request.RequestedFormatMode = m_settings.FormatMode() == "check" ? FormatMode::Check : FormatMode::Apply;
		request.DestructiveActionConfirmed = m_settings.ConfirmClean();

		if (operationId == "quality.format")
		{
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(operationId.toStdString(), request);
			const bool formatNeedsAttention = plan.Toolchain.ClangFormatPath.empty() || plan.FormatSourceFiles.empty();
			QVBoxLayout* maintenanceLayout = AddDetailsGroup(
			    layout,
			    formatNeedsAttention ? "Action Dependencies - Needs action" : "Action Dependencies - Ready",
			    "Formatting depends on clang-format being installed and source files being discoverable in Engine/ and Projects/.",
			    false);
			AddStatusRow(
			    *maintenanceLayout,
			    "clang-format",
			    plan.Toolchain.ClangFormatPath.empty() ? "Missing" : "Ready",
			    plan.Toolchain.ClangFormatPath.empty() ? "clang-format was not found." : "clang-format is available.",
			    plan.Toolchain.ClangFormatPath.empty() ? "warning" : "ok",
			    CreateActionDependencyActions("toolchain.check", "Verify Host Environment"));
			AddStatusRow(
			    *maintenanceLayout,
			    "Source files",
			    plan.FormatSourceFiles.empty() ? "None found" : "Ready",
			    QString("Discovered %1 source files eligible for formatting.").arg(plan.FormatSourceFiles.size()),
			    plan.FormatSourceFiles.empty() ? "warning" : "ok");
			return;
		}

		if (operationId == "workspace.clean")
		{
			QVBoxLayout* maintenanceLayout = AddDetailsGroup(
			    layout,
			    m_settings.ConfirmClean() ? "Action Dependencies - Ready" : "Action Dependencies - Confirmation required",
			    "Cleaning generated outputs does not require the build toolchain, but destructive scopes still require explicit confirmation.",
			    false);
			AddStatusRow(
			    *maintenanceLayout,
			    "Confirmation",
			    m_settings.ConfirmClean() ? "Enabled" : "Required on run",
			    m_settings.ConfirmClean() ? "Clean confirmation is enabled in settings." : "The launcher will ask for confirmation before destructive clean actions run.",
			    m_settings.ConfirmClean() ? "ok" : "warning");
		}
	}

	QVBoxLayout* LauncherMainWindow::AddInlineOptionsSection(QVBoxLayout& layout)
	{
		QFrame* section = new QFrame(this);
		section->setObjectName("InlineOptionsSection");
		QVBoxLayout* sectionLayout = new QVBoxLayout(section);
		sectionLayout->setContentsMargins(0, 0, 0, 0);
		sectionLayout->setSpacing(4);
		layout.addWidget(section);
		return sectionLayout;
	}

	void LauncherMainWindow::AddNoOptionsMessage(QVBoxLayout& layout, const QString& text)
	{
		QLabel* label = new QLabel(text, this);
		label->setObjectName("MutedLabel");
		label->setAccessibleName(text);
		label->setWordWrap(true);
		layout.addWidget(label);
	}

	void LauncherMainWindow::SetControlsEnabled(bool enabled)
	{
		if (m_cleanButton != nullptr)
		{
			m_cleanButton->setEnabled(enabled);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setEnabled(enabled);
		}
		if (m_optionsStack != nullptr)
		{
			m_optionsStack->setVisible(enabled);
		}
	}

	bool LauncherMainWindow::ShouldShowActionSpecificCleanButton(const QString& operationId) const
	{
		return SupportsActionSpecificClean(operationId);
	}

	bool LauncherMainWindow::SupportsActionSpecificClean(const QString& operationId) const
	{
		return operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build") ||
		    operationId == "cook.tools.prepare" || operationId == "cook.project" || operationId == "cook.shaders" || operationId == "cook.textures" ||
		    operationId == "cook.assets";
	}

	QVector<LauncherCleanTarget> LauncherMainWindow::BuildActionSpecificCleanTargets(const QString& operationId) const
	{
		QVector<LauncherCleanTarget> targets;
		const QString projectId = m_projectModel.SelectedProjectId();
		if ((operationId.startsWith("project.build") || operationId.startsWith("cook.")) && projectId.isEmpty())
		{
			return targets;
		}

		const QString editorProfile = m_settings.EditorProfile();
		const QString runtimeProfile = m_settings.RuntimeProfile();
		const auto addNamedTargets = [this, &targets](const QString& profileName, const QStringList& targetNames, const QString& detail) {
			for (const QString& targetName : targetNames)
			{
				if (!targetName.isEmpty())
				{
					AddTargetArtifactOutputs(targets, m_repositoryRoot, profileName, targetName, detail);
				}
			}
		};
		const auto addProjectArtifacts = [this, &addNamedTargets, &targets](const QString& profileName, const QString& projectName, const QString& detail) {
			const std::optional<BuildProfile> profile = FindBuildProfile(profileName.toStdString());
			if (!profile.has_value())
			{
				return;
			}
			const QString productRole = profile->Target == BuildProfileTarget::Game ? "runtime" : "editor";
			AddProjectTargetArtifactOutputs(
			    targets,
			    m_repositoryRoot,
			    profileName,
			    projectName,
			    productRole,
			    QString::fromStdString(BuildProjectTargetName(projectName.toStdString(), *profile)),
			    detail);
		};

		if (operationId == "launcher.build.self" || operationId == "workspace.build-all")
		{
			const std::filesystem::path runningLauncherPath = std::filesystem::path(QCoreApplication::applicationFilePath().toStdString());
			AddTargetArtifactOutputs(
			    targets,
			    m_repositoryRoot,
			    editorProfile,
			    "SparkleLauncher",
			    "Launcher direct build outputs. The currently running launcher executable is preserved until restart.",
			    runningLauncherPath);
			AddTargetArtifactOutputs(targets, m_repositoryRoot, editorProfile, "SparkleLauncherProbe", "Launcher probe binary and matching direct build outputs.");
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore library",
			    GetSymbolDirectory(m_repositoryRoot) / "launcher" / editorProfile.toStdString() / "lib" / "SparkleLauncherCore.lib",
			    "Launcher support library built for the selected editor profile.");
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore program database",
			    GetSymbolDirectory(m_repositoryRoot) / "launcher" / editorProfile.toStdString() / "lib" / "SparkleLauncherCore.pdb",
			    "Launcher support library debug symbols built for the selected editor profile.");
		}

		if (operationId == "project.build.editor" || operationId == "workspace.build-all")
		{
			if (operationId == "project.build.editor" && !m_settings.SelectedTargets().trimmed().isEmpty())
			{
				addNamedTargets(editorProfile, m_settings.SelectedTargets().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts), "Selected editor build target outputs.");
			}
			else
			{
				addProjectArtifacts(editorProfile, projectId, "Selected project editor target outputs.");
			}
		}

		if (operationId == "project.build.runtime" || operationId == "workspace.build-all")
		{
			if (operationId == "project.build.runtime" && !m_settings.SelectedTargets().trimmed().isEmpty())
			{
				addNamedTargets(runtimeProfile, m_settings.SelectedTargets().split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts), "Selected runtime build target outputs.");
			}
			else
			{
				addProjectArtifacts(runtimeProfile, projectId, "Selected project runtime target outputs.");
			}
		}

		if (operationId == "cook.tools.prepare" || operationId == "workspace.build-all")
		{
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddTargetArtifactOutputs(targets, m_repositoryRoot, editorProfile, "AssetCooker", "AssetCooker executable outputs.");
			AddTargetArtifactOutputs(targets, m_repositoryRoot, editorProfile, "TextureCooker", "TextureCooker executable outputs.");
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
			AddTargetArtifactOutputs(targets, m_repositoryRoot, editorProfile, "ShaderCompiler", "ShaderCompiler executable outputs.");
#endif
		}

		if (operationId == "cook.project")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked project content",
			    GetCookedProjectDirectory(m_repositoryRoot, projectId.toStdString()),
			    "All cooked content for the selected project.");
			AddExplicitCleanTarget(
			    targets,
			    "Shader cache",
			    GetBuildDirectory(m_repositoryRoot) / "Cache" / "Shaders",
			    "Shared local shader cache refreshed by cook operations.");
		}
		else if (operationId == "cook.shaders")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Shader cache",
			    GetBuildDirectory(m_repositoryRoot) / "Cache" / "Shaders",
			    "Shared local shader cache refreshed by shader cooking.");
		}
		else if (operationId == "cook.textures" || operationId == "cook.assets")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked project content",
			    GetCookedProjectDirectory(m_repositoryRoot, projectId.toStdString()),
			    operationId == "cook.textures" ? "Selected project cooked texture outputs." : "Selected project cooked mesh and material outputs.");
		}

		return targets;
	}

	LauncherOperationRequest LauncherMainWindow::BuildCleanOperationRequest(const QString& operationId) const
	{
		LauncherOperationRequest request = BuildOperationRequest("workspace.clean");
		request.CleanTargets = BuildActionSpecificCleanTargets(operationId);
		request.ConfirmClean = false;
		return request;
	}

	LauncherOperationRequest LauncherMainWindow::BuildScopedCleanRequest(const QString& cleanScope) const
	{
		LauncherOperationRequest request = BuildOperationRequest("workspace.clean");
		request.CleanScope = cleanScope;
		request.CleanTargets.clear();
		request.ConfirmClean = false;
		return request;
	}

	LauncherOperationRequest LauncherMainWindow::BuildDependencyCleanRequest(const ThirdPartyDependencyUiEntry& dependency) const
	{
		LauncherOperationRequest request = BuildOperationRequest("workspace.clean");
		LauncherCleanTarget target;
		target.DisplayName = dependency.Label + " dependency cache";
		target.Path = QString::fromStdString((GetBuildDirectory(m_repositoryRoot) / "_deps" / dependency.CacheDirectoryName.toStdString()).string());
		target.Detail = "Local cache folder for " + dependency.Label + " " + dependency.Version + ".";
		request.CleanTargets.clear();
		request.CleanTargets.push_back(target);
		request.ConfirmClean = false;
		return request;
	}

	LauncherOperationRequest LauncherMainWindow::BuildDependencyRegenerateRequest() const
	{
		LauncherOperationRequest request = BuildOperationRequest("workspace.setup");
		request.ForceConfigure = true;
		return request;
	}

	QWidget* LauncherMainWindow::CreateTrackedDependencyActions(const ThirdPartyDependencyUiEntry& dependency)
	{
		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    dependency.Label + " actions",
		    "Dependency actions",
		    {
		        LauncherActionMenuEntry{
		            "Refresh this cache",
		            [this, dependency]() { TriggerDependencyRegenerate(dependency); }},
		        LauncherActionMenuEntry{
		            "Clean this cache",
		            [this, dependency]() { TriggerDependencyClean(dependency); }},
		    });
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateDisabledSourceTierActions(const DependencyGroupUiEntry& group)
	{
		QVector<LauncherActionMenuEntry> entries;
		if (!group.ConfigureOption.isEmpty())
		{
			entries.push_back(LauncherActionMenuEntry{
			    "Copy enable option",
			    [this, option = group.ConfigureOption]() {
				    QGuiApplication::clipboard()->setText("-D" + option + "=ON");
				    SetStatusMessage("Copied source tier enable option.");
			    }});
		}
		entries.push_back(LauncherActionMenuEntry{
		    "Open source tier guide",
		    [this]() {
			    OpenLocalPath(m_repositoryRoot / "docs" / "dependency-capability-tiers.md");
		    }});
		entries.push_back(LauncherActionMenuEntry{
		    "Open reconfigure workflow",
		    [this]() {
			    TriggerActionDependencyRegenerate("workspace.generate-solution", "Open Reconfigure Workflow", true);
		    }});

		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    group.Label + " configuration actions",
		    "Tier actions",
		    entries);
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateActionDependencyActions(
	    const QString& actionId,
	    const QString& actionTitle,
	    const QString& cleanScope,
	    const QString& cleanTitle,
	    bool navigateInsteadOfRun)
	{
		QVector<LauncherActionMenuEntry> entries;
		entries.push_back(LauncherActionMenuEntry{
		    actionTitle,
		    [this, actionId, actionTitle, navigateInsteadOfRun]() {
			    TriggerActionDependencyRegenerate(actionId, actionTitle, navigateInsteadOfRun);
		    }});
		if (!cleanScope.isEmpty())
		{
			entries.push_back(LauncherActionMenuEntry{
			    "Clean",
			    [this, cleanScope, cleanTitle]() {
				    TriggerActionDependencyClean(cleanScope, cleanTitle);
			    }});
		}
		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    actionTitle + " actions",
		    "Dependency actions",
		    entries);
		RegisterFocusable(button);
		return button;
	}

	QWidget* LauncherMainWindow::CreateFolderShortcutActions()
	{
		const LauncherStatePaths statePaths = GetLauncherStatePaths(m_repositoryRoot);
		const QVector<LauncherActionMenuEntry> entries = {
		    LauncherActionMenuEntry{
		        "Open artifacts",
		        [this]() {
			        OpenLocalPath(GetArtifactDirectory(m_repositoryRoot));
		        }},
		    LauncherActionMenuEntry{
		        "Open packages",
		        [this]() {
			        OpenLocalPath(m_repositoryRoot / "dist");
		        }},
		    LauncherActionMenuEntry{
		        "Open launcher logs",
		        [this, statePaths]() {
			        OpenLocalPath(statePaths.LogsDirectory);
		        }},
		};

		QToolButton* button = CreateLauncherOverflowActionButton(
		    this,
		    "Declared workspace folder shortcuts",
		    "Open folders",
		    entries);
		button->setObjectName("HeaderUtilityButton");
		button->setText("Folders");
		button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		button->setAutoRaise(false);
		button->setToolTip("Open declared generated roots: artifacts, dist, and launcher logs.");
		button->setAccessibleDescription(button->toolTip());
		button->setMinimumSize(82, 24);
		button->setMaximumSize(96, 26);
		RegisterFocusable(button);
		return button;
	}

	void LauncherMainWindow::OpenLocalPath(const std::filesystem::path& path)
	{
		if (!PathExists(path) && !DirectoryHasEntries(path))
		{
			SetStatusMessage("Evidence target is not available yet.");
			return;
		}

		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path.string())));
		SetStatusMessage("Opened evidence target.");
	}

	void LauncherMainWindow::TriggerActionDependencyClean(const QString& cleanScope, const QString& cleanTitle)
	{
		LauncherOperationRequest request = BuildScopedCleanRequest(cleanScope);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Clean canceled");
			return;
		}

		StartOperation(std::move(request), cleanTitle);
	}

	void LauncherMainWindow::TriggerActionDependencyRegenerate(const QString& actionId, const QString& actionTitle, bool navigateInsteadOfRun)
	{
		if (navigateInsteadOfRun)
		{
			SetSelectedOperation(actionId);
			SetStatusMessage("Selected " + actionTitle + ". Configure parameters if needed, then run.");
			return;
		}

		LauncherOperationRequest request = BuildOperationRequest(actionId);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Regenerate canceled");
			return;
		}

		StartOperation(std::move(request), actionTitle);
	}

	void LauncherMainWindow::TriggerDependencyClean(const ThirdPartyDependencyUiEntry& dependency)
	{
		LauncherOperationRequest request = BuildDependencyCleanRequest(dependency);
		if (!ConfirmRunRequest(request))
		{
			SetStatusMessage("Clean canceled");
			return;
		}

		StartOperation(std::move(request), "Clean " + dependency.Label);
	}

	void LauncherMainWindow::TriggerDependencyRegenerate(const ThirdPartyDependencyUiEntry& dependency)
	{
		const QMessageBox::StandardButton regenerateResult = QMessageBox::question(
		    this,
		    "Refresh Dependency Cache",
		    QStringLiteral("This will clean only the cached %1 source folder first, then run the source-tier sync workflow to refill any missing enabled tier content. Continue?").arg(dependency.Label),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (regenerateResult != QMessageBox::Ok)
		{
			SetStatusMessage("Regenerate canceled");
			return;
		}

		LauncherOperationRequest cleanRequest = BuildDependencyCleanRequest(dependency);
		if (!ConfirmRunRequest(cleanRequest))
		{
			SetStatusMessage("Regenerate canceled");
			return;
		}

		LauncherOperationRequest setupRequest = BuildDependencyRegenerateRequest();
		const QString title = "Regenerate " + dependency.Label;
		const QString cleanTitle = "Clean cache before " + title;
		const QString runId = QStringLiteral("run-%1").arg(m_nextRunIndex + 1, 4, 10, QChar('0'));
		PendingFollowUpOperation followUp;
		followUp.Request = std::move(setupRequest);
		followUp.Title = title;
		m_pendingFollowUpOperations.insert(runId, std::move(followUp));
		StartOperation(std::move(cleanRequest), cleanTitle);
	}

	void LauncherMainWindow::RebuildOptionsPages()
	{
		if (m_optionsStack == nullptr || m_isRebuildingOptions)
		{
			return;
		}

		int preservedVerticalScroll = 0;
		int preservedHorizontalScroll = 0;
		if (QScrollArea* currentScrollArea = qobject_cast<QScrollArea*>(m_optionsStack->currentWidget()))
		{
			if (QScrollBar* verticalScrollBar = currentScrollArea->verticalScrollBar())
			{
				preservedVerticalScroll = verticalScrollBar->value();
			}
			if (QScrollBar* horizontalScrollBar = currentScrollArea->horizontalScrollBar())
			{
				preservedHorizontalScroll = horizontalScrollBar->value();
			}
		}

		m_isRebuildingOptions = true;

		while (m_optionsStack->count() > 0)
		{
			QWidget* page = m_optionsStack->widget(0);
			m_optionsStack->removeWidget(page);
			page->deleteLater();
		}
		m_optionsPageByOperation.clear();

		EnsureOptionsPage(m_selectedOperationId);
		if (m_optionsPageByOperation.contains(m_selectedOperationId))
		{
			m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(m_selectedOperationId));
			if (QScrollArea* rebuiltScrollArea = qobject_cast<QScrollArea*>(m_optionsStack->currentWidget()))
			{
				if (QScrollBar* verticalScrollBar = rebuiltScrollArea->verticalScrollBar())
				{
					verticalScrollBar->setValue(preservedVerticalScroll);
				}
				if (QScrollBar* horizontalScrollBar = rebuiltScrollArea->horizontalScrollBar())
				{
					horizontalScrollBar->setValue(preservedHorizontalScroll);
				}
			}
		}

		m_projectSelectors.clear();
		for (QComboBox* combo : findChildren<QComboBox*>())
		{
			if (combo != nullptr && combo->property("ProjectSelector").toBool())
			{
				m_projectSelectors.push_back(combo);
			}
		}

		m_isRebuildingOptions = false;
	}

	void LauncherMainWindow::EnsureOptionsPage(const QString& operationId)
	{
		if (m_optionsStack == nullptr || operationId.isEmpty() || m_optionsPageByOperation.contains(operationId))
		{
			return;
		}

		const int pageIndex = m_optionsStack->addWidget(CreateOptionsPage(operationId, m_optionsStack));
		m_optionsPageByOperation.insert(operationId, pageIndex);
		m_optionsStack->setCurrentIndex(pageIndex);
	}

	void LauncherMainWindow::LoadActionHistory()
	{
		m_actionHistory.clear();

		const std::filesystem::path historyPath = GetLauncherStatePaths(m_repositoryRoot).ActionHistoryPath;
		std::ifstream stream(historyPath);
		if (!stream.is_open())
		{
			return;
		}

		std::string line;
		while (std::getline(stream, line))
		{
			const QStringList fields = QString::fromStdString(line).split('\t');
			if (fields.size() < 4)
			{
				continue;
			}

			ActionHistoryRecord record;
			record.CompletedAtUtc = fields[1].trimmed();
			record.ResultText = fields[2].trimmed();
			bool exitCodeOk = false;
			record.ExitCode = fields[3].trimmed().toInt(&exitCodeOk);
			if (!exitCodeOk)
			{
				record.ExitCode = -1;
			}
			m_actionHistory.insert(fields[0].trimmed(), record);
		}
	}

	void LauncherMainWindow::SaveActionHistory() const
	{
		const LauncherStatePaths statePaths = GetLauncherStatePaths(m_repositoryRoot);
		std::error_code errorCode;
		std::filesystem::create_directories(statePaths.RootDirectory, errorCode);

		std::ofstream stream(statePaths.ActionHistoryPath, std::ios::out | std::ios::trunc);
		if (!stream.is_open())
		{
			return;
		}

		for (auto it = m_actionHistory.constBegin(); it != m_actionHistory.constEnd(); ++it)
		{
			stream << SanitizeActionHistoryField(it.key()).toStdString() << '\t'
			       << SanitizeActionHistoryField(it.value().CompletedAtUtc).toStdString() << '\t'
			       << SanitizeActionHistoryField(it.value().ResultText).toStdString() << '\t'
			       << it.value().ExitCode << '\n';
		}
	}

	void LauncherMainWindow::UpdateActionHistoryDisplay()
	{
		if (m_lastRunSummaryLabel == nullptr || m_lastRunResultLabel == nullptr)
		{
			return;
		}

		const auto found = m_actionHistory.constFind(m_selectedOperationId);
		if (found == m_actionHistory.constEnd())
		{
			m_lastRunSummaryLabel->setText("No recorded run for this workflow yet.");
			m_lastRunResultLabel->setText("Result data will persist between launcher sessions.");
			if (m_dismissHistoryButton != nullptr)
			{
				m_dismissHistoryButton->setEnabled(false);
				m_dismissHistoryButton->setVisible(false);
			}
			return;
		}

		const QDateTime completedAt = QDateTime::fromString(found->CompletedAtUtc, Qt::ISODate);
		const QString completedAtText = completedAt.isValid() ? completedAt.toLocalTime().toString("MMMM d, yyyy HH:mm") : found->CompletedAtUtc;
		const bool failed = found->ExitCode != 0;
		m_lastRunSummaryLabel->setText(QStringLiteral("%1 on %2").arg(failed ? "Last failed" : "Last completed", completedAtText));
		if (failed)
		{
			const QString recoveryHint = FailureRecoveryHint(m_selectedOperationId, found->ResultText);
			m_lastRunResultLabel->setText(QStringLiteral("Recovery: %1").arg(recoveryHint.isEmpty() ? found->ResultText : recoveryHint));
		}
		else
		{
			m_lastRunResultLabel->setText(QStringLiteral("Result: %1").arg(found->ResultText));
		}
		if (m_dismissHistoryButton != nullptr)
		{
			m_dismissHistoryButton->setEnabled(true);
			m_dismissHistoryButton->setVisible(true);
		}
	}

	void LauncherMainWindow::DismissSelectedActionHistory()
	{
		if (m_selectedOperationId.isEmpty())
		{
			return;
		}

		if (m_actionHistory.contains(m_selectedOperationId))
		{
			m_actionHistory.remove(m_selectedOperationId);
			SaveActionHistory();
			UpdateActionHistoryDisplay();
			RebuildOptionsPages();
			SetStatusMessage("Dismissed stored workflow attention. Raw logs remain available.");
		}
	}

	void LauncherMainWindow::LoadLauncherIconFont()
	{
#ifdef SPARKLE_FONT_AWESOME_SOLID_TTF
		const char* fontPath = SPARKLE_FONT_AWESOME_SOLID_TTF;
		std::error_code errorCode;
		if (!std::filesystem::exists(fontPath, errorCode) || errorCode)
		{
			return;
		}

		const int fontId = QFontDatabase::addApplicationFont(QString::fromUtf8(fontPath));
		if (fontId < 0)
		{
			return;
		}

		const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
		if (!families.empty())
		{
			m_iconFontFamily = families.front();
		}
#endif
	}

	QIcon LauncherMainWindow::CreateApplicationIcon() const
	{
		QIcon icon;
		for (const int size : {16, 24, 32, 48, 64})
		{
			QPixmap pixmap(size, size);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			const QRectF bounds(1.0, 1.0, size - 2.0, size - 2.0);
			const qreal radius = qMax(3.0, size * 0.18);
			painter.setPen(QColor("#3f4d35"));
			painter.setBrush(QColor("#151713"));
			painter.drawRoundedRect(bounds, radius, radius);

			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor("#76b900"));
			painter.drawRoundedRect(QRectF(size * 0.18, size * 0.18, size * 0.64, size * 0.16), radius * 0.45, radius * 0.45);
			painter.setBrush(QColor("#dff3cf"));
			painter.drawEllipse(QRectF(size * 0.62, size * 0.62, size * 0.18, size * 0.18));

			QFont font("Segoe UI");
			font.setBold(true);
			font.setPixelSize(qMax(10, static_cast<int>(size * 0.48)));
			painter.setFont(font);
			painter.setPen(QColor("#f0f3f6"));
			painter.drawText(QRectF(0, size * 0.16, size, size * 0.72), Qt::AlignCenter, "S");
			icon.addPixmap(pixmap);
		}

		return icon;
	}

	QString LauncherMainWindow::IconGlyph(LauncherIcon icon) const
	{
		switch (icon)
		{
		case LauncherIcon::Start:
			return QChar(0xf135);
		case LauncherIcon::Setup:
			return QChar(0xf0ad);
		case LauncherIcon::Build:
			return QChar(0xf6e3);
		case LauncherIcon::Cook:
			return QChar(0xf466);
		case LauncherIcon::Run:
			return QChar(0xf04b);
		case LauncherIcon::Package:
			return QChar(0xf466);
		case LauncherIcon::System:
			return QChar(0xf108);
		case LauncherIcon::Maintain:
			return QChar(0xf1de);
		case LauncherIcon::Queued:
			return QChar(0xf017);
		case LauncherIcon::Running:
			return QChar(0xf04b);
		case LauncherIcon::Done:
			return QChar(0xf00c);
		case LauncherIcon::Failed:
			return QChar(0xf071);
		case LauncherIcon::Copy:
			return QChar(0xf0c5);
		case LauncherIcon::Overflow:
			return QChar(0xf142);
		}

		return QString();
	}

	QIcon LauncherMainWindow::CreateLauncherIcon(LauncherIcon icon, const QColor& color) const
	{
		if (m_iconFontFamily.isEmpty())
		{
			return {};
		}

		QPixmap pixmap(kLauncherIconSize, kLauncherIconSize);
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::TextAntialiasing, true);
		QFont iconFont(m_iconFontFamily);
		iconFont.setPixelSize(kLauncherIconSize - 1);
		painter.setFont(iconFont);
		painter.setPen(color);
		painter.drawText(pixmap.rect(), Qt::AlignCenter, IconGlyph(icon));
		return QIcon(pixmap);
	}

	QIcon LauncherMainWindow::WorkflowIconForKey(const QString& iconKey) const
	{
		if (iconKey == "home")
		{
			return CreateLauncherIcon(LauncherIcon::Start, QColor(kColorStateQueued));
		}
		if (iconKey == "launch")
		{
			return CreateLauncherIcon(LauncherIcon::Run, QColor(kColorStateQueued));
		}
		if (iconKey == "sync")
		{
			return CreateLauncherIcon(LauncherIcon::Setup, QColor(kColorStateQueued));
		}
		if (iconKey == "build")
		{
			return CreateLauncherIcon(LauncherIcon::Build, QColor(kColorStateQueued));
		}
		if (iconKey == "cook")
		{
			return CreateLauncherIcon(LauncherIcon::Cook, QColor(kColorStateQueued));
		}
		if (iconKey == "test")
		{
			return CreateLauncherIcon(LauncherIcon::Done, QColor(kColorStateQueued));
		}
		if (iconKey == "package")
		{
			return CreateLauncherIcon(LauncherIcon::Package, QColor(kColorStateQueued));
		}
		if (iconKey == "system")
		{
			return CreateLauncherIcon(LauncherIcon::System, QColor(kColorStateQueued));
		}
		if (iconKey == "maintain")
		{
			return CreateLauncherIcon(LauncherIcon::Maintain, QColor(kColorStateQueued));
		}
		return {};
	}

	QIcon LauncherMainWindow::ActivityIconForState(RunState state) const
	{
		switch (state)
		{
		case RunState::Queued:
			return CreateLauncherIcon(LauncherIcon::Queued, QColor(kColorStateQueued));
		case RunState::Running:
			return CreateLauncherIcon(LauncherIcon::Running, QColor(kColorStateRunning));
		case RunState::Done:
			return CreateLauncherIcon(LauncherIcon::Done, QColor(kColorStateSuccess));
		case RunState::Failed:
			return CreateLauncherIcon(LauncherIcon::Failed, QColor(kColorStateDestructive));
		}

		return {};
	}

	void LauncherMainWindow::RegisterFocusable(QWidget* widget)
	{
		if (widget == nullptr)
		{
			return;
		}

		widget->setFocusPolicy(Qt::StrongFocus);
		m_tabOrderWidgets.push_back(widget);
	}

	void LauncherMainWindow::SetActiveWorkflowGroup(int workflowIndex)
	{
		if (m_workflowGroupButtonGroup == nullptr)
		{
			return;
		}

		for (QAbstractButton* button : m_workflowGroupButtonGroup->buttons())
		{
			const bool active = button != nullptr && button->property("WorkflowIndex").toInt() == workflowIndex;
			button->setProperty("ActiveState", active ? "true" : "false");
			button->style()->unpolish(button);
			button->style()->polish(button);
			button->update();
		}
	}

	void LauncherMainWindow::ConfigureTabOrder()
	{
		QWidget* previousWidget = nullptr;
		for (QWidget* widget : m_tabOrderWidgets)
		{
			if (widget == nullptr)
			{
				continue;
			}

			if (previousWidget != nullptr)
			{
				setTabOrder(previousWidget, widget);
			}
			previousWidget = widget;
		}
	}

	void LauncherMainWindow::UpdateRunAvailability()
	{
		if (m_runButton == nullptr || m_cleanButton == nullptr)
		{
			return;
		}

		if (m_selectedOperationId.isEmpty())
		{
			const QString reason = "Select a workflow before running.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("Select a workflow before cleaning generated outputs.");
			m_cleanButton->setAccessibleDescription("Select a workflow before cleaning generated outputs.");
			return;
		}

		if (m_selectedOperationId == LauncherHomeOperationId())
		{
			const QString reason = "Use Quick Start cards for the next best action.";
			m_cleanButton->setVisible(false);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("Home summarizes readiness and does not own generated outputs.");
			m_cleanButton->setAccessibleDescription(m_cleanButton->toolTip());
			m_runButton->setVisible(false);
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (m_selectedOperationId == LauncherSystemOperationId())
		{
			const QString reason = "This page is for inspection. Use workflow pages for executable actions.";
			m_cleanButton->setVisible(false);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip(reason);
			m_cleanButton->setAccessibleDescription(reason);
			m_runButton->setVisible(false);
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		m_runButton->setVisible(true);
		m_cleanButton->setVisible(ShouldShowActionSpecificCleanButton(m_selectedOperationId));

		if (OperationNeedsProject(m_selectedOperationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			const QString reason = "No project discovered. Confirm this is a Sparkle repository or package root with Projects/<Project> markers, then run Generate Build Files if rebuilding from source.";
			m_runButton->setEnabled(false);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			m_cleanButton->setEnabled(false);
			m_cleanButton->setToolTip("No project discovered for this clean action.");
			m_cleanButton->setAccessibleDescription("No project discovered for this clean action.");
			return;
		}

		const QVector<LauncherCleanTarget> cleanTargets = SupportsActionSpecificClean(m_selectedOperationId) ? BuildActionSpecificCleanTargets(m_selectedOperationId) : QVector<LauncherCleanTarget>();
		const bool canClean = !cleanTargets.isEmpty();
		m_cleanButton->setEnabled(canClean);
		m_cleanButton->setToolTip(canClean ? "Clean only the generated outputs tied to " + DisplayNameForOperation(m_selectedOperationId) + "." : "Clean is not available for this workflow.");
		m_cleanButton->setAccessibleDescription(m_cleanButton->toolTip());

		if (FindBuildWorkspaceOperationDefinition(m_selectedOperationId.toStdString()).has_value())
		{
			const BuildWorkspaceOperationRequest request = MakeWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(m_selectedOperationId.toStdString(), request);
			const QString reason = plan.CanRun ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going." : FirstBlockingReadinessMessage(plan);
			m_runButton->setEnabled(plan.CanRun);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		if (m_selectedOperationId == "quality.format")
		{
			MaintenanceOperationRequest request;
			request.RepositoryRoot = m_repositoryRoot;
			request.ProjectId = m_projectModel.SelectedProjectId().isEmpty() ? std::string("Showcase") : m_projectModel.SelectedProjectId().toStdString();
			request.EditorProfile = m_settings.EditorProfile().toStdString();
			request.RequestedFormatMode = m_settings.FormatMode() == "check" ? FormatMode::Check : FormatMode::Apply;
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(m_selectedOperationId.toStdString(), request);
			const QString reason = plan.CanRun ? "Run " + DisplayNameForOperation(m_selectedOperationId) + ". Existing runs keep going." : FirstBlockingReadinessMessage(plan.ReadinessMessages);
			m_runButton->setEnabled(plan.CanRun);
			m_runButton->setToolTip(reason);
			m_runButton->setAccessibleDescription(reason);
			return;
		}

		const QString title = DisplayNameForOperation(m_selectedOperationId);
		const QString actionDescription = "Run " + title + ". Existing runs keep going.";
		m_runButton->setEnabled(true);
		m_runButton->setToolTip(actionDescription);
		m_runButton->setAccessibleDescription(actionDescription);
	}

	const LauncherOperationDescriptor* LauncherMainWindow::FindOperationDescriptor(const QString& operationId) const
	{
		for (const LauncherOperationDescriptor& operation : m_backend.Operations())
		{
			if (operation.Id == operationId)
			{
				return &operation;
			}
		}
		return nullptr;
	}

	QString LauncherMainWindow::DisplayNameForOperation(const QString& operationId) const
	{
		const QString overrideName = LauncherOperationDisplayNameOverride(operationId);
		if (!overrideName.isEmpty())
		{
			return overrideName;
		}
		const LauncherOperationDescriptor* operation = FindOperationDescriptor(operationId);
		return operation == nullptr ? operationId : operation->DisplayName;
	}

	bool LauncherMainWindow::OperationNeedsProject(const QString& operationId) const
	{
		if (operationId == "workspace.clean")
		{
			return m_settings.CleanScope().contains("selected-cooked");
		}

		return operationId.startsWith("project.") || operationId.startsWith("cook.");
	}

	bool LauncherMainWindow::OperationNeedsConfirmation(const QString& operationId) const
	{
		if (operationId.startsWith("cook."))
		{
			return m_settings.ForceRecook() && !m_settings.ConfirmForceRecook();
		}
		if (operationId == "workspace.clean")
		{
			return false;
		}

		return false;
	}

	QString LauncherMainWindow::FailureRecoveryHint(const QString& operationId, const QString& statusText) const
	{
		if (OperationNeedsProject(operationId) && m_projectModel.SelectedProjectId().isEmpty())
		{
			return "No project is selected. Confirm the repository/package root contains Projects/<Project> markers, then regenerate project files if rebuilding from source.";
		}
		if (operationId.startsWith("cook.") && OperationNeedsConfirmation(operationId))
		{
			return "Enable Confirm clean cook, then retry.";
		}
		if (operationId.startsWith("project.build") || statusText.contains("cmake", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("tool", Qt::CaseInsensitive))
		{
			return "Run Sync > Verify Host Environment, then retry this workflow.";
		}
		if (statusText.contains("Rider", Qt::CaseInsensitive))
		{
			return "Install Rider or switch the IDE selector back to Visual Studio, then retry.";
		}
		if (statusText.contains("disabled in this workspace configuration", Qt::CaseInsensitive) ||
		    statusText.contains("No cook tool groups are enabled", Qt::CaseInsensitive))
		{
			return "This workflow is disabled by the current dependency-group configuration. Reconfigure the workspace with the matching group enabled, then sync and build again.";
		}
		if (statusText.contains("shader package", Qt::CaseInsensitive) || statusText.contains("shader", Qt::CaseInsensitive))
		{
			return "Run Cook > Cook Shaders, then retry this workflow.";
		}
		if (statusText.contains("executable is missing", Qt::CaseInsensitive) || statusText.contains("missing", Qt::CaseInsensitive))
		{
			const bool runtimeLaunch = operationId == "project.open.runtime" || ((operationId == "project.run" || operationId == "project.run.smoke") && m_settings.LaunchTarget() == "runtime");
			if ((operationId == "project.open.runtime" || operationId == "project.run" || operationId == "project.run.smoke") && runtimeLaunch)
			{
				return "Run Build > Build Runtime, then retry this workflow.";
			}
			if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
			{
				return "Run Build > Build Editor, then retry this workflow.";
			}
		}

		if (operationId.startsWith("cook."))
		{
			return "Review the output below. If tools or cooked inputs are missing, run Build Cooking Tools before retrying.";
		}

		if (FindLaunchOperationDefinition(operationId.toStdString()).has_value())
		{
			return "Review the output below. If package binaries are missing, use a complete runtime package; if source artifacts are missing, build the matching target before retrying.";
		}

		return "Review the output below, adjust the selected options, then retry.";
	}

	LauncherOperationRequest LauncherMainWindow::BuildOperationRequest(const QString& operationId) const
	{
		LauncherOperationRequest request;
		request.RepositoryRoot = m_repositoryRoot;
		request.OperationId = operationId;
		request.ProjectId = m_projectModel.SelectedProjectId();
		request.EditorProfile = m_settings.EditorProfile();
		request.RuntimeProfile = m_settings.RuntimeProfile();
		request.WorkspaceIde = m_settings.WorkspaceIde();
		request.SelectedTargets = m_settings.SelectedTargets();
		request.ShaderPackages = m_settings.ShaderPackages();
		request.ShaderTargets = ResolveShaderTargetSelection(m_settings);
		request.ShaderBackend = m_settings.ShaderBackend();
		request.ShaderCacheDirectory = m_settings.ShaderCacheDirectory();
		request.ShaderUseCache = m_settings.ShaderUseCache();
		request.ShaderEnableDebugInfo = m_settings.ShaderEnableDebugInfo();
		request.ShaderEnableOptimizations = m_settings.ShaderEnableOptimizations();
		request.ShaderWarningsAsErrors = m_settings.ShaderWarningsAsErrors();
		request.ShaderStripReflection = m_settings.ShaderStripReflection();
		request.ShaderStripDebugInfo = m_settings.ShaderStripDebugInfo();
		request.ShaderWriteDebugArtifacts = m_settings.ShaderWriteDebugArtifacts();
		request.ShaderWriteCookedShaderStats = m_settings.ShaderWriteCookedShaderStats();
		request.ShaderDebugArtifactDirectory = m_settings.ShaderWriteDebugArtifacts() ?
		                                          ResolvedShaderDebugArtifactDirectory(m_repositoryRoot, m_projectModel, m_settings) :
		                                          QString();
		request.LaunchBackend = m_settings.LaunchBackend();
		request.LaunchTarget = m_settings.LaunchTarget();
		request.LaunchVSync = m_settings.LaunchVSync();
		request.LaunchHighPerformanceAdapter = m_settings.LaunchHighPerformanceAdapter();
		request.LaunchMeshAutoBatching = m_settings.LaunchMeshAutoBatching();
		request.LaunchCommandLineArguments = m_settings.LaunchCommandLineArguments();
		request.LaunchCVars = m_settings.LaunchCVars();
		request.SmokeBackend = m_settings.SmokeBackend();
		request.SmokeFrameLimit = m_settings.SmokeFrameLimit();
		request.FormatMode = m_settings.FormatMode();
		request.CleanScope = m_settings.CleanScope();
		request.LaunchSmokeTest = m_settings.LaunchSmokeTest();
		request.ForceConfigure = m_settings.ForceConfigure();
		request.ForceRecook = m_settings.ForceRecook();
		request.ConfirmForceRecook = m_settings.ConfirmForceRecook();
		request.ConfirmClean = m_settings.ConfirmClean();
		request.SmokeTrace = m_settings.SmokeTrace();
		request.SmokeSkipLevelSwitching = m_settings.SmokeSkipLevelSwitching();
		if (operationId == "project.open.editor")
		{
			request.LaunchTarget = "editor";
			request.LaunchSmokeTest = false;
		}
		else if (operationId == "project.open.runtime")
		{
			request.LaunchTarget = "runtime";
			request.LaunchSmokeTest = false;
		}
		else if (operationId == "project.run.smoke")
		{
			request.LaunchSmokeTest = true;
		}
		else if (operationId == "project.run")
		{
			request.LaunchSmokeTest = false;
		}
		return request;
	}

	bool LauncherMainWindow::ConfirmRunRequest(LauncherOperationRequest& request) const
	{
		const bool cleanRequested = request.OperationId == "workspace.clean";
		const bool customCleanRequested = cleanRequested && !request.CleanTargets.isEmpty();
		const bool destructiveRequested = request.ForceRecook || cleanRequested;
		if (!destructiveRequested)
		{
			return true;
		}
		if (request.ForceRecook && !request.ConfirmForceRecook)
		{
			QMessageBox::warning(
			    const_cast<LauncherMainWindow*>(this),
			    "Confirmation Required",
			    "Enable Confirm clean cook before removing cooked outputs.");
			return false;
		}
		if (cleanRequested && !request.ConfirmClean)
		{
			QStringList scopeNames;
			if (customCleanRequested)
			{
				for (const LauncherCleanTarget& target : request.CleanTargets)
				{
					scopeNames.push_back(target.DisplayName + "\n" + target.Path);
				}
			}
			else
			{
				for (const QString& scopeValue : request.CleanScope.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
				{
					scopeNames.push_back(CleanScopeDisplayName(scopeValue));
				}
			}

			QString message = customCleanRequested ? "Generated outputs to clean:\n\n" + scopeNames.join("\n\n") : "Clean scopes:\n" + scopeNames.join('\n');
			if (!request.ProjectId.isEmpty())
			{
				message += "\nProject: " + request.ProjectId;
			}
			message += customCleanRequested ? "\n\nThis removes only the generated outputs mapped to the selected action. Continue?" :
			                                 "\n\nThis removes generated files for the selected scope. Continue?";
			const QMessageBox::StandardButton result = QMessageBox::question(
			    const_cast<LauncherMainWindow*>(this),
			    customCleanRequested ? "Confirm Action Clean" : "Confirm Clean Workspace",
			    message,
			    QMessageBox::Ok | QMessageBox::Cancel,
			    QMessageBox::Cancel);
			if (result != QMessageBox::Ok)
			{
				return false;
			}

			request.ConfirmClean = true;
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    const_cast<LauncherMainWindow*>(this),
		    "Confirm Clean Cook",
		    "This workflow will remove cooked outputs before cooking. Continue?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::No);
		return result == QMessageBox::Yes;
	}

	void LauncherMainWindow::PromptForLauncherRestart()
	{
		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Launcher Rebuilt",
		    "Sparkle Launcher was rebuilt successfully. Restart now to run the new binary?",
		    QMessageBox::Yes | QMessageBox::No,
		    QMessageBox::Yes);
		if (result != QMessageBox::Yes)
		{
			SetStatusMessage("Launcher rebuilt. Restart it when you're ready.");
			return;
		}

		const std::filesystem::path relaunchedExecutablePath =
		    GetLauncherArtifactDirectory(m_repositoryRoot, m_settings.EditorProfile().toStdString()) /
		    std::filesystem::path(QCoreApplication::applicationFilePath().toStdString()).filename();
		const QString executablePath = QString::fromStdString(relaunchedExecutablePath.string());
		const bool started = QProcess::startDetached(executablePath, {});
		if (!started)
		{
			QMessageBox::warning(
			    this,
			    "Restart Failed",
			    "The rebuilt launcher is ready, but the restart command could not be started.");
			SetStatusMessage("Launcher rebuilt, but automatic restart failed.");
			return;
		}

		QCoreApplication::quit();
	}

	bool LauncherMainWindow::OfferWorkspacePrerequisiteOperation(const QString& operationId)
	{
		BuildWorkspaceOperationRequest request = MakeWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
		const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId.toStdString(), request);
		if (plan.CanRun)
		{
			return true;
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			prerequisiteOperationId = "toolchain.check";
			promptTitle = "Verify Host Environment";
			promptAction = "Required host prerequisites are missing. Run Sync > Verify Host Environment now?";
		}
		else if ((operationId == "workspace.open-solution" || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare") && !plan.Freshness.Current)
		{
			prerequisiteOperationId = "workspace.generate-solution";
			promptTitle = "Generate Build Files";
			promptAction = "Generated build files are not current. Run Generate Build Files now?";
		}
		else if (operationId == "workspace.open-solution")
		{
			prerequisiteOperationId = "toolchain.check";
			promptTitle = "Verify Host Environment";
			promptAction = QString("%1 is not currently available. Run Sync > Verify Host Environment now and verify the Visual Studio, Qt, and optional ClangCL toolchain?").arg(SelectedWorkspaceIdeName(m_settings));
		}
		else
		{
			return true;
		}

		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			readiness.push_back(QString::fromStdString(message));
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Prerequisite Missing",
		    promptAction + (readiness.isEmpty() ? QString() : "\n\n" + readiness.join('\n')),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), promptTitle);
		return false;
	}

	bool LauncherMainWindow::OfferLaunchPrerequisiteOperation(const QString& operationId)
	{
		LauncherOperationRequest request = BuildOperationRequest(operationId);
		LaunchOperationRequest launchRequest;
		launchRequest.RepositoryRoot = request.RepositoryRoot;
		launchRequest.ProjectId = request.ProjectId.toStdString();
		launchRequest.EditorProfile = request.EditorProfile.toStdString();
		launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		launchRequest.Target = request.LaunchTarget.toStdString();
		launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
		launchRequest.VSync = request.LaunchVSync.toStdString();
		launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
		launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
		for (const QString& argument : QProcess::splitCommand(request.LaunchCommandLineArguments))
		{
			if (!argument.isEmpty())
			{
				launchRequest.CustomArguments.push_back(argument.toStdString());
			}
		}
		launchRequest.CustomCVars.clear();
		for (const QString& part : request.LaunchCVars.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				launchRequest.CustomCVars.push_back(trimmed.toStdString());
			}
		}
		launchRequest.SmokeBackend = request.SmokeBackend.toStdString();
		launchRequest.SmokeFrameLimit = request.SmokeFrameLimit.toStdString();
		launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
		launchRequest.SmokeTrace = request.SmokeTrace;
		launchRequest.SmokeSkipLevelSwitching = request.SmokeSkipLevelSwitching;

		const LaunchOperationPlan plan = PlanLaunchOperation(operationId.toStdString(), launchRequest);
		if (plan.CanRun)
		{
			return true;
		}

		bool executableMissing = false;
		bool cookedMeshesMissing = false;
		bool cookedTexturesMissing = false;
		bool cookedShadersMissing = false;
		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			const QString readinessMessage = QString::fromStdString(message);
			readiness.push_back(readinessMessage);
			executableMissing = executableMissing || readinessMessage.contains("Executable is missing", Qt::CaseInsensitive);
			cookedMeshesMissing = cookedMeshesMissing || readinessMessage.contains("Cooked scene assets are missing", Qt::CaseInsensitive);
			cookedTexturesMissing = cookedTexturesMissing || readinessMessage.contains("Cooked textures are missing", Qt::CaseInsensitive);
			cookedShadersMissing = cookedShadersMissing || readinessMessage.contains("Cooked shaders are missing", Qt::CaseInsensitive);
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (executableMissing)
		{
			const bool runtimeTarget = request.LaunchTarget == "runtime";
			prerequisiteOperationId = runtimeTarget ? "project.build.runtime" : "project.build.editor";
			promptTitle = runtimeTarget ? "Build Runtime" : "Build Editor";
			promptAction = "The executable is missing. Start " + promptTitle + " now?";
		}
		else if (cookedMeshesMissing || cookedTexturesMissing || cookedShadersMissing)
		{
			const int missingCount = static_cast<int>(cookedMeshesMissing) + static_cast<int>(cookedTexturesMissing) + static_cast<int>(cookedShadersMissing);
			if (missingCount == 1)
			{
				if (cookedMeshesMissing)
				{
					prerequisiteOperationId = "cook.assets";
					promptTitle = "Cook Scenes And Meshes";
					promptAction = "Cooked scenes and meshes are missing. Start Cook Scenes And Meshes now?";
				}
				else if (cookedTexturesMissing)
				{
					prerequisiteOperationId = "cook.textures";
					promptTitle = "Cook Textures";
					promptAction = "Cooked textures are missing. Start Cook Textures now?";
				}
				else
				{
					prerequisiteOperationId = "cook.shaders";
					promptTitle = "Cook Shaders";
					promptAction = "Cooked shaders are missing. Start Cook Shaders now?";
				}
			}
			else
			{
				prerequisiteOperationId = "cook.project";
				promptTitle = "Cook All";
				promptAction = "Multiple cooked asset sets are missing. Start Cook All now?";
			}
		}
		else
		{
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Launch Prerequisite Missing",
		    promptAction + "\n\n" + readiness.join('\n'),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			SetStatusMessage("Launch canceled");
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			SetStatusMessage("Prerequisite run canceled");
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), DisplayNameForOperation(prerequisiteOperationId));
		return false;
	}

	bool LauncherMainWindow::OfferCookPrerequisiteOperation(const QString& operationId)
	{
		LauncherOperationRequest request = BuildOperationRequest(operationId);
		CookOperationRequest cookRequest;
		cookRequest.RepositoryRoot = request.RepositoryRoot;
		cookRequest.ProjectId = request.ProjectId.toStdString();
		cookRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
		cookRequest.Mode = request.ForceRecook ? CookMode::Force : CookMode::Incremental;
		cookRequest.ForceRecookConfirmed = request.ConfirmForceRecook;
		for (const QString& part : request.ShaderPackages.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts))
		{
			const QString trimmed = part.trimmed();
			if (!trimmed.isEmpty())
			{
				cookRequest.ShaderPackages.push_back(trimmed.toStdString());
			}
		}

		const CookOperationPlan plan = PlanCookOperation(operationId.toStdString(), cookRequest);
		if (plan.CanRun)
		{
			return true;
		}

		bool workspaceMissing = false;
		bool cookToolsMissing = false;
		bool dependencyGroupDisabled = false;
		QStringList readiness;
		for (const std::string& message : plan.ReadinessMessages)
		{
			const QString readinessMessage = QString::fromStdString(message);
			readiness.push_back(readinessMessage);
			workspaceMissing = workspaceMissing || readinessMessage.contains("Run Generate Build Files first", Qt::CaseInsensitive);
			cookToolsMissing = cookToolsMissing || readinessMessage.contains("run Build Cooking Tools first", Qt::CaseInsensitive);
			dependencyGroupDisabled = dependencyGroupDisabled || readinessMessage.contains("disabled in this workspace configuration", Qt::CaseInsensitive) ||
			    readinessMessage.contains("No cook tool groups are enabled", Qt::CaseInsensitive);
		}

		if (dependencyGroupDisabled)
		{
			QMessageBox::information(
			    this,
			    "Cook Workflow Disabled",
			    "This cook workflow is disabled by the current workspace dependency-group configuration.\n\n" + readiness.join('\n'));
			return false;
		}

		QString prerequisiteOperationId;
		QString promptTitle;
		QString promptAction;
		if (workspaceMissing)
		{
			prerequisiteOperationId = "workspace.generate-solution";
			promptTitle = "Generate Build Files";
			promptAction = "Generated build files are not current. Run Generate Build Files now?";
		}
		else if (cookToolsMissing)
		{
			prerequisiteOperationId = "cook.tools.prepare";
			promptTitle = "Build Cooking Tools";
			promptAction = "Required cooking tools are missing. Run Build Cooking Tools now?";
		}
		else
		{
			return true;
		}

		const QMessageBox::StandardButton result = QMessageBox::question(
		    this,
		    "Cook Prerequisite Missing",
		    promptAction + "\n\n" + readiness.join('\n'),
		    QMessageBox::Ok | QMessageBox::Cancel,
		    QMessageBox::Ok);
		if (result != QMessageBox::Ok)
		{
			return false;
		}

		LauncherOperationRequest prerequisiteRequest = BuildOperationRequest(prerequisiteOperationId);
		if (!ConfirmRunRequest(prerequisiteRequest))
		{
			return false;
		}

		StartOperation(std::move(prerequisiteRequest), DisplayNameForOperation(prerequisiteOperationId));
		return false;
	}

	void LauncherMainWindow::StartOperation(LauncherOperationRequest request, const QString& title)
	{
		request.RunId = QStringLiteral("run-%1").arg(++m_nextRunIndex, 4, 10, QChar('0'));
		RegisterRun(request.RunId, title);
		SetStatusMessage("Starting " + title);
		m_backend.RunOperation(std::move(request));
	}

	void LauncherMainWindow::SetStatusMessage(const QString& message)
	{
		Q_UNUSED(message);
	}

	void LauncherMainWindow::SetSelectedOperation(const QString& operationId)
	{
		m_selectedOperationId = operationId;
		const QString title = DisplayNameForOperation(operationId);
		SetControlsEnabled(true);
		UpdateActionHistoryDisplay();
		const bool isStaticPage = operationId == LauncherHomeOperationId() || operationId == LauncherSystemOperationId();
		if (m_activeOperationLabel != nullptr)
		{
			m_activeOperationLabel->setText(title);
			m_activeOperationLabel->setVisible(true);
		}
		if (m_actionMetaPanel != nullptr)
		{
			m_actionMetaPanel->setVisible(!isStaticPage);
		}
		if (m_runButton != nullptr)
		{
			m_runButton->setText(PrimaryActionLabelForOperationId(operationId));
		}
		if (m_optionsStack != nullptr)
		{
			EnsureOptionsPage(operationId);
			if (m_optionsPageByOperation.contains(operationId))
			{
				m_optionsStack->setCurrentIndex(m_optionsPageByOperation.value(operationId));
			}
		}

		if (m_processButtonGroup != nullptr)
		{
			for (QAbstractButton* button : m_processButtonGroup->buttons())
			{
				button->setChecked(button->property("OperationId").toString() == operationId);
			}
		}

		if (m_operationStack != nullptr && m_workflowPageByOperation.contains(operationId))
		{
			const int workflowIndex = m_workflowPageByOperation.value(operationId);
			m_lastOperationByWorkflowIndex.insert(workflowIndex, operationId);
			m_operationStack->setCurrentIndex(workflowIndex);
			SetActiveWorkflowGroup(workflowIndex);
			const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
			if (workflowIndex >= 0 && workflowIndex < workflows.size())
			{
				m_operationStack->setVisible(workflows[workflowIndex].OperationIds.size() > 1);
			}
		}

		UpdateRunAvailability();
	}

	void LauncherMainWindow::RegisterRun(const QString& runId, const QString& title)
	{
		++m_startedRunCount;
		QListWidgetItem* item = new QListWidgetItem(m_activityList);
		item->setData(Qt::UserRole, runId);
		item->setSizeHint(QSize(0, 34));
		item->setText(QString());
		const LauncherActivityRowWidgets rowWidgets = CreateLauncherActivityRow(m_activityList, title);
		m_activityList->setItemWidget(item, rowWidgets.Root);
		m_runItems.insert(runId, item);
		m_runItemWidgets.insert(runId, {rowWidgets.Root, rowWidgets.Indicator, rowWidgets.TitleLabel, rowWidgets.StateLabel});
		m_runTitles.insert(runId, title);
		SetRunState(runId, RunState::Queued, title);
		m_runOutputs.insert(runId, title + " queued.\n");
		m_activityList->setCurrentItem(item);
		m_activeRunId = runId;
		UpdateProgress();
	}

	void LauncherMainWindow::SetRunState(const QString& runId, RunState state, const QString& title)
	{
		QListWidgetItem* item = m_runItems.value(runId, nullptr);
		if (item == nullptr)
		{
			return;
		}

		m_runStates.insert(runId, state);
		m_runTitles.insert(runId, title);

		QString stateText;
		QColor stateColor;
		switch (state)
		{
		case RunState::Queued:
			stateText = "Queued";
			stateColor = QColor(kColorStateQueued);
			break;
		case RunState::Running:
			stateText = "Running";
			stateColor = QColor(kColorStateRunning);
			break;
		case RunState::Done:
			stateText = "Done";
			stateColor = QColor(kColorStateSuccess);
			break;
		case RunState::Failed:
			stateText = "Failed";
			stateColor = QColor(kColorStateDestructive);
			break;
		}

		item->setText(QString());
		item->setIcon(ActivityIconForState(state));
		item->setData(Qt::UserRole + 1, stateText);
		item->setData(Qt::AccessibleTextRole, stateText + ": " + title);
		item->setData(Qt::AccessibleDescriptionRole, "Launcher activity run " + stateText.toLower());
		item->setToolTip(stateText + ": " + title);

		const ActivityRunWidgets widgets = m_runItemWidgets.value(runId);
		if (widgets.Root != nullptr)
		{
			widgets.Root->setProperty("RunState", stateText.toLower());
			if (widgets.Indicator != nullptr)
			{
				widgets.Indicator->setProperty("RunState", stateText.toLower());
				widgets.Indicator->style()->unpolish(widgets.Indicator);
				widgets.Indicator->style()->polish(widgets.Indicator);
			}
			if (widgets.TitleLabel != nullptr)
			{
				widgets.TitleLabel->setText(title);
			}
			if (widgets.StateLabel != nullptr)
			{
				widgets.StateLabel->setText(stateText);
			}
			widgets.Root->style()->unpolish(widgets.Root);
			widgets.Root->style()->polish(widgets.Root);
		}
		UpdateActivityRunSelectionVisuals();
	}

	void LauncherMainWindow::AppendRunOutput(const QString& runId, const QString& text)
	{
		QString output = m_runOutputs.value(runId);
		output += text;
		const int overflowCharacters = output.size() - kMaxOperationOutputCharacters;
		if (overflowCharacters > 0)
		{
			output.remove(0, overflowCharacters);
		}
		m_runOutputs.insert(runId, output);
	}

	void LauncherMainWindow::ShowRunOutput(const QString& runId)
	{
		m_activeRunId = runId;
		UpdateActivityRunSelectionVisuals();
		const RunState state = m_runStates.value(runId, RunState::Queued);
		const QString title = m_runTitles.value(runId, "Selected run");
		if (m_selectedRunSummary != nullptr)
		{
			switch (state)
			{
			case RunState::Queued:
				m_selectedRunSummary->setText("Queued: " + title + ". Waiting to start.");
				break;
			case RunState::Running:
				m_selectedRunSummary->setText("Running: " + title + ". Output is updating below.");
				break;
			case RunState::Done:
				m_selectedRunSummary->setText("Done: " + title + ". Output is available below.");
				break;
			case RunState::Failed:
				m_selectedRunSummary->setText("Failed: " + title + ". Review the summary and raw output below.");
				break;
			}
		}
		if (m_operationOutput != nullptr)
		{
			const bool compactOutput = state == RunState::Done;
			m_operationOutput->setMinimumHeight(compactOutput ? kOperationOutputMinHeight : kOperationOutputProminentMinHeight);
			m_operationOutput->setMaximumHeight(compactOutput ? kOperationOutputCompactMaxHeight : kOperationOutputMaxHeight);
			m_operationOutput->setPlainText(m_runOutputs.value(runId));
			m_operationOutput->moveCursor(QTextCursor::End);
		}
		if (m_copyOutputButton != nullptr)
		{
			const bool canCopyOutput = m_activityLogExpanded && m_operationOutput != nullptr && !m_operationOutput->toPlainText().isEmpty();
			m_copyOutputButton->setEnabled(canCopyOutput);
			m_copyOutputButton->setToolTip(canCopyOutput ? "Copy output for the selected run. Shortcut: Ctrl+Shift+C." : "Select a run to copy its output. Shortcut: Ctrl+Shift+C.");
		}
	}

	void LauncherMainWindow::SetActivityLogExpanded(bool expanded)
	{
		m_activityLogExpanded = expanded;
		if (m_activityDetailsPanel != nullptr)
		{
			m_activityDetailsPanel->setVisible(expanded);
		}
		if (m_operationOutput != nullptr)
		{
			m_operationOutput->setVisible(expanded);
		}
		if (m_toggleOutputButton != nullptr)
		{
			m_toggleOutputButton->setText(expanded ? "Hide raw log" : "Show raw log");
			m_toggleOutputButton->setToolTip(expanded ? "Hide raw process output for the selected run." : "Show raw process output for the selected run.");
			m_toggleOutputButton->setAccessibleDescription(m_toggleOutputButton->toolTip());
		}
		if (m_copyOutputButton != nullptr)
		{
			const bool canCopyOutput = expanded && m_operationOutput != nullptr && !m_operationOutput->toPlainText().isEmpty();
			m_copyOutputButton->setEnabled(canCopyOutput);
		}
	}

	void LauncherMainWindow::UpdateActivityRunSelectionVisuals()
	{
		for (auto it = m_runItemWidgets.begin(); it != m_runItemWidgets.end(); ++it)
		{
			const bool isSelected = it.key() == m_activeRunId;
			if (it.value().Root != nullptr)
			{
				it.value().Root->setProperty("Selected", isSelected);
				it.value().Root->style()->unpolish(it.value().Root);
				it.value().Root->style()->polish(it.value().Root);
			}
		}
	}

	void LauncherMainWindow::UpdateProgress()
	{
		if (m_activityDetailsPanel == nullptr)
		{
			return;
		}

		const bool hasRuns = m_startedRunCount > 0;
		if (m_activityDetailsPanel != nullptr)
		{
			m_activityDetailsPanel->setVisible(hasRuns && m_activityLogExpanded);
		}
		if (m_copyOutputButton != nullptr && !hasRuns)
		{
			m_copyOutputButton->setEnabled(false);
		}
	}

	void LauncherMainWindow::PopulateProjectSelectors()
	{
		for (QComboBox* combo : m_projectSelectors)
		{
			if (combo != nullptr)
			{
				PopulateProjectCombo(*combo);
			}
		}
	}

	void LauncherMainWindow::PopulateProjectCombo(QComboBox& combo) const
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		if (m_projectModel.Projects().empty())
		{
			combo.addItem("No projects found", "");
			combo.setToolTip("No projects were discovered. Confirm the selected root contains Projects/<Project> markers or inspect project discovery output.");
			combo.setEnabled(false);
			return;
		}

		combo.setEnabled(true);
		combo.setToolTip("Project used by this workflow.");
		for (const LauncherProjectSummary& project : m_projectModel.Projects())
		{
			combo.addItem(project.DisplayName, project.Id);
			combo.setItemData(combo.count() - 1, project.Id + "\n" + QString::fromStdString(project.RootPath.string()), Qt::ToolTipRole);
		}

		const int selectedIndex = combo.findData(m_projectModel.SelectedProjectId());
		combo.setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
	}

	void LauncherMainWindow::ApplyVisualStyle()
	{
		ApplyLauncherVisualStyle(*this);
	}
}
