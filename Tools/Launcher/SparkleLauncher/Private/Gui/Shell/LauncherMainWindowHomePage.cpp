#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPageUtilities.h"
#include "LauncherProjectModel.h"
#include "LauncherRecoveryUiModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kStatusChipColumnWidth = LauncherUi::Row::StatusChipColumnWidth;
	static constexpr int kStatusActionColumnWidth = LauncherUi::Row::StatusActionColumnWidth;
	static constexpr const char* kColorStateReady = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
		const BuildWorkspaceOperationRequest workspaceRequest = BuildWorkspacePlanRequest(m_repositoryRoot, m_projectModel, m_settings);
		const BuildWorkspaceOperationPlan packagePlan = PlanBuildWorkspaceOperation("package.release", workspaceRequest);
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const std::filesystem::path releaseRoot = m_repositoryRoot / "dist" / "releases";
		const bool sourceRoot = PathExists(m_repositoryRoot / "CMakeLists.txt");
		const bool packageRoot = PathExists(m_repositoryRoot / "SparkleLauncher.exe") && DirectoryHasEntries(m_repositoryRoot / "manifests");

		const auto planLaunch = [this](const QString& operationId) {
			LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, operationId);
			LaunchOperationRequest launchRequest;
			launchRequest.RepositoryRoot = request.RepositoryRoot;
			launchRequest.OperationId = operationId.toStdString();
			launchRequest.ProjectId = request.ProjectId.toStdString();
			launchRequest.EditorProfile = request.EditorProfile.toStdString();
			launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
			launchRequest.Target = request.LaunchTarget.toStdString();
			launchRequest.StartupLevel = request.LaunchStartupLevel.toStdString();
			launchRequest.EnableSmokeTest = request.LaunchSmokeTest;
			launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
			launchRequest.VSync = request.LaunchVSync.toStdString();
			launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
			launchRequest.MeshAutoBatching = request.LaunchMeshAutoBatching.toStdString();
			launchRequest.PreferPartitionedTlas = request.LaunchPreferPartitionedTlas.toStdString();
			launchRequest.PtlasOperationWriterPath = request.LaunchPtlasOperationWriterPath.toStdString();
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
			launchRequest.SmokeViewMode = request.SmokeViewMode.toStdString();
			launchRequest.SmokeCapturePath = request.SmokeCapturePath.toStdString();
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
		const int storedFailureCount = m_actionHistory.FailureCount();
		const auto createOpenButton = [this](const QString& label, const std::filesystem::path& path) {
			QPushButton* button = new QPushButton(label, this);
			button->setObjectName("CommandSecondaryButton");
			button->setMinimumHeight(LauncherUi::Button::SecondaryMinHeight);
			button->setToolTip("Open the referenced file or folder.");
			button->setAccessibleName(label);
			RegisterFocusable(button);
			connect(button, &QPushButton::clicked, this, [this, path]() {
				OpenLocalPath(path);
			});
			return button;
		};
		QWidget* quickStartBody = new QWidget(this);
		quickStartBody->setObjectName("QuickStartBody");
		quickStartBody->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		QVBoxLayout* bodyLayout = new QVBoxLayout(quickStartBody);
		bodyLayout->setContentsMargins(LauncherUi::Home::BodyLeft, LauncherUi::Home::BodyTop, LauncherUi::Home::BodyRight, LauncherUi::Home::BodyBottom);
		bodyLayout->setSpacing(LauncherUi::Home::SectionSpacing);

		const auto addHomeSection = [bodyLayout](const QString& title) {
			QLabel* section = new QLabel(title);
			section->setObjectName("CommandSectionTitle");
			section->setAccessibleName(title);
			bodyLayout->addWidget(section);
		};

		const QString launchProvenance = packageRoot ? "bundled package components" : "local source artifacts";
		const QString heroTitle = "Explore Project";
		const QString heroDetail = QStringLiteral("Choose Editor or Runtime below. Products use the Launch Project settings, including startup level %1, and launch from %2 when available.")
		                               .arg(m_settings.LaunchStartupLevel(), launchProvenance);
		layout.addWidget(CreateHomeHeroCard(
		    m_repositoryRoot,
		    heroTitle,
		    heroDetail,
		    "neutral",
		    nullptr,
		    nullptr,
		    "showcase-hero.png",
		    this));

		ResponsiveCardGridWidget* libraryGrid = new ResponsiveCardGridWidget(
		    LauncherUi::Home::ProductCardMinWidth,
		    LauncherUi::Home::ProductCardMaxWidth,
		    LauncherUi::Home::ProductCardMaxColumns,
		    LauncherUi::Home::TileSpacing,
		    LauncherUi::Home::TileSpacing,
		    quickStartBody);

		const QString editorStatus = editorPlan.CanRun ? "Ready" : (editorExecutableMissing ? "Missing" : "Blocked");
		const QString editorDetail = editorPlan.CanRun ?
		    QStringLiteral("Launch the selected project editor from %1.").arg(launchProvenance) :
		    "Editor output is not ready yet. Build the editor target to unlock this launch path.";
		const QString editorCardOperationId = editorPlan.CanRun ? "project.open.editor" : "project.build.editor";
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Editor",
		    editorStatus,
		    editorDetail,
		    editorPlan.CanRun ? "ok" : "warning",
		    CreateCommandActionButton(editorCardOperationId, editorPlan.CanRun ? "Open Editor" : "Build Editor", false, editorPlan.CanRun),
		    "library",
		    "showcase-editor.png",
		    this));
		const QString runtimeStatus = runtimePlan.CanRun ? "Ready" : (runtimeExecutableMissing ? "Missing" : "Blocked");
		const QString runtimeDetail = runtimePlan.CanRun ?
		    QStringLiteral("Run the selected project runtime from %1.").arg(launchProvenance) :
		    "Runtime output is not ready yet. Build the runtime target to unlock the standalone path.";
		const QString runtimeCardOperationId = runtimePlan.CanRun ? "project.open.runtime" : "project.build.runtime";
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Runtime",
		    runtimeStatus,
		    runtimeDetail,
		    runtimePlan.CanRun ? "ok" : "warning",
		    CreateCommandActionButton(runtimeCardOperationId, runtimePlan.CanRun ? "Open Runtime" : "Build Runtime", false, runtimePlan.CanRun),
		    "library",
		    "showcase-runtime.png",
		    this));
		if (libraryGrid->CardCount() > 0)
		{
			addHomeSection("Products");
			bodyLayout->addWidget(libraryGrid);
		}
		else
		{
			delete libraryGrid;
		}

		addHomeSection("Discover");
		ResponsiveCardGridWidget* discoverGrid = new ResponsiveCardGridWidget(
		    LauncherUi::Home::DiscoverCardMinWidth,
		    LauncherUi::Home::DiscoverCardMaxWidth,
		    LauncherUi::Home::DiscoverCardMaxColumns,
		    LauncherUi::Home::TileSpacing,
		    LauncherUi::Home::TileSpacing,
		    quickStartBody);

		const bool packagePresent = DirectoryHasEntries(releaseRoot);
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Architecture",
		    architectureDoc ? "Available" : "Pending",
		    "Product boundaries, artifact layout, packaging model, and launcher workflow intent.",
		    architectureDoc ? "ok" : "warning",
		    architectureDoc ? createOpenButton("Open Architecture", m_repositoryRoot / "docs" / "plans" / "build-artifacts-release-architecture-roadmap.md") : nullptr,
		    "discover",
		    "sparkle-architecture.png",
		    this));
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Dependency Tiers",
		    dependencyDoc ? "Available" : "Pending",
		    readyDependencyCount == enabledDependencyCount ? "Enabled source tiers are cached; optional tiers unlock more build and cook capability." :
		                                                  "Sync source tiers only when you need the extra local build or cook capability.",
		    dependencyDoc ? "ok" : "warning",
		    dependencyDoc ? createOpenButton("Open Tiers", m_repositoryRoot / "docs" / "dependency-capability-tiers.md") : CreateCommandActionButton("workspace.sync-source-tiers", "Sync Source Tiers", false),
		    "discover",
		    "sparkle-source-tiers.png",
		    this));
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Validation",
		    validationReport ? "Available" : (storedFailureCount == 0 ? "No active issue" : "Review activity"),
		    validationReport ? "Open the latest final validation report." :
		                       "Run smoke tests or open Activity when you want runtime confidence.",
		    validationReport || storedFailureCount == 0 ? "ok" : "warning",
		    validationReport ? createOpenButton("Open Report", validationReportPath) : CreateCommandActionButton("project.run.smoke", "Run Smoke Test", false),
		    "discover",
		    "sparkle-validation.png",
		    this));
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Package",
		    packagePresent ? "Present" : (packagePlan.CanRun ? "Ready" : "Blocked"),
		    packagePresent ? "Open assembled release folders." : "Assemble a release package from artifacts; publishing remains separate.",
		    packagePresent || packagePlan.CanRun ? "ok" : "warning",
		    packagePresent ? createOpenButton("Open Packages", releaseRoot) : CreateCommandActionButton("package.release", "Assemble", false),
		    "discover",
		    "sparkle-package.png",
		    this));
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Content",
		    missingCookDomains == 0 ? "Ready" : QStringLiteral("%1 missing").arg(missingCookDomains),
		    missingCookDomains == 0 ? "Cooked content is ready for launch workflows." : "Cook only the missing generated content when local artifacts need it.",
		    missingCookDomains == 0 ? "ok" : "warning",
		    CreateCommandActionButton("cook.project", missingCookDomains == 0 ? QStringLiteral("Cook All") : QStringLiteral("Cook Missing"), false),
		    "discover",
		    "showcase-content.png",
		    this));
		discoverGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Tools",
		    uxDoc ? "Available" : "Pending",
		    "Open UX notes or continue into Sync, Build, Cook, Test, Package, and Maintain workflows from the rail.",
		    uxDoc ? "ok" : "warning",
		    uxDoc ? createOpenButton("Open UX Notes", m_repositoryRoot / "docs" / "plans" / "launcher-principal-ux-concept.md") : nullptr,
		    "discover",
		    "sparkle-tools.png",
		    this));
		bodyLayout->addWidget(discoverGrid);
		layout.addWidget(quickStartBody);
	}
}

