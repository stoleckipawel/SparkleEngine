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
#include "LauncherContentModel.h"
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
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		const bool packageRoot =
		    PathExists(m_repositoryRoot / "SparkleLauncher.exe") && DirectoryHasEntries(m_repositoryRoot / "manifests");

		const auto planLaunch = [this](const QString& operationId)
		{
			LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, operationId);
			LaunchOperationRequest launchRequest;
			launchRequest.RepositoryRoot = request.RepositoryRoot;
			launchRequest.OperationId = operationId.toStdString();
			launchRequest.ContentId = request.ContentId.toStdString();
			launchRequest.EditorProfile = request.EditorProfile.toStdString();
			launchRequest.RuntimeProfile = request.RuntimeProfile.toStdString();
			launchRequest.Target = request.LaunchTarget.toStdString();
			launchRequest.StartupLevel = request.LaunchStartupLevel.toStdString();
			launchRequest.GraphicsBackend = request.LaunchBackend.toStdString();
			launchRequest.VSync = request.LaunchVSync.toStdString();
			launchRequest.PreferHighPerformanceAdapter = request.LaunchHighPerformanceAdapter.toStdString();
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
			return PlanLaunchOperation(operationId.toStdString(), launchRequest);
		};

		const LaunchOperationPlan editorPlan = planLaunch("launch.editor");
		const LaunchOperationPlan runtimePlan = planLaunch("launch.runtime");
		const bool editorExecutableMissing = !editorPlan.Readiness.ExecutableReady;
		const bool runtimeExecutableMissing = !runtimePlan.Readiness.ExecutableReady;
		const bool editorCanBuild = !editorPlan.CanRun && editorExecutableMissing;
		const bool runtimeCanBuild = !runtimePlan.CanRun && runtimeExecutableMissing;
		const SourceDependencyInventoryStatus dependencyStatus = InspectSourceDependencyCache(dependencyCachePath);
		QWidget* quickStartBody = new QWidget(this);
		quickStartBody->setObjectName("QuickStartBody");
		quickStartBody->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		QVBoxLayout* bodyLayout = new QVBoxLayout(quickStartBody);
		bodyLayout->setContentsMargins(
		    LauncherUi::Home::BodyLeft,
		    LauncherUi::Home::BodyTop,
		    LauncherUi::Home::BodyRight,
		    LauncherUi::Home::BodyBottom);
		bodyLayout->setSpacing(LauncherUi::Home::SectionSpacing);

		const auto addHomeSection = [bodyLayout](const QString& title)
		{
			QLabel* section = new QLabel(title);
			section->setObjectName("CommandSectionTitle");
			section->setAccessibleName(title);
			bodyLayout->addWidget(section);
		};

		const QString launchProvenance = packageRoot ? "bundled package components" : "local source artifacts";
		const QString heroTitle = "Explore Sparkle";
		const QString heroDetail =
		    QStringLiteral("Choose Editor or Runtime below. Products use the header startup level %1 and launch from %2 when available.")
		        .arg(ResolveStartupLevelDisplayName(), launchProvenance);
		layout.addWidget(
		    CreateHomeHeroCard(m_repositoryRoot, heroTitle, heroDetail, "neutral", nullptr, nullptr, "showcase-hero.png", this));

		ResponsiveCardGridWidget* libraryGrid = new ResponsiveCardGridWidget(
		    LauncherUi::Home::ProductCardMinWidth,
		    LauncherUi::Home::ProductCardMaxWidth,
		    LauncherUi::Home::ProductCardMaxColumns,
		    LauncherUi::Home::TileSpacing,
		    LauncherUi::Home::TileSpacing,
		    quickStartBody);

		const QString editorStatus = editorPlan.CanRun ? "Ready" : (editorExecutableMissing ? "Missing" : "Blocked");
		const QString editorDetail = editorPlan.CanRun
		    ? QStringLiteral("Launch the editor from %1.").arg(launchProvenance)
		    : (editorExecutableMissing ? "Editor output is not ready yet. Build the editor target to unlock this launch path."
		                               : "Editor output exists. Resolve the remaining cooked-content prerequisites to launch.");
		const QString editorCardOperationId = editorCanBuild ? "workspace.build.editor" : "launch.editor";
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Editor",
		    editorStatus,
		    editorDetail,
		    editorPlan.CanRun ? "ok" : "warning",
		    CreateCommandActionButton(
		        editorCardOperationId,
		        editorPlan.CanRun ? "Open Editor" : (editorCanBuild ? "Build Editor" : "Resolve"),
		        false,
		        editorPlan.CanRun || !editorCanBuild),
		    "library",
		    "showcase-editor.png",
		    this));
		const QString runtimeStatus = runtimePlan.CanRun ? "Ready" : (runtimeExecutableMissing ? "Missing" : "Blocked");
		const QString runtimeDetail = runtimePlan.CanRun
		    ? QStringLiteral("Run the runtime from %1.").arg(launchProvenance)
		    : (runtimeExecutableMissing ? "Runtime output is not ready yet. Build the runtime target to unlock the standalone path."
		                                : "Runtime output exists. Resolve the remaining cooked-content prerequisites to launch.");
		const QString runtimeCardOperationId = runtimeCanBuild ? "workspace.build.runtime" : "launch.runtime";
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Runtime",
		    runtimeStatus,
		    runtimeDetail,
		    runtimePlan.CanRun ? "ok" : "warning",
		    CreateCommandActionButton(
		        runtimeCardOperationId,
		        runtimePlan.CanRun ? "Open Runtime" : (runtimeCanBuild ? "Build Runtime" : "Resolve"),
		        false,
		        runtimePlan.CanRun || !runtimeCanBuild),
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

		layout.addWidget(quickStartBody);
	}
}
