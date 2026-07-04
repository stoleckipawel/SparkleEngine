#include "LauncherProjectModel.h"

#include "SparkleLauncher/LauncherProjectDefaults.h"
#include "SparkleLauncher/ProjectDiscovery.h"

namespace SparkleLauncher
{
	static bool ProjectsMatch(const QVector<LauncherProjectSummary>& left, const QVector<LauncherProjectSummary>& right)
	{
		if (left.size() != right.size())
		{
			return false;
		}

		for (int index = 0; index < left.size(); ++index)
		{
			if (left[index].Id != right[index].Id || left[index].DisplayName != right[index].DisplayName || left[index].RootPath != right[index].RootPath)
			{
				return false;
			}
		}

		return true;
	}

	LauncherProjectModel::LauncherProjectModel(QObject* parent)
	    : QObject(parent)
	{
	}

	const QVector<LauncherProjectSummary>& LauncherProjectModel::Projects() const
	{
		return m_projects;
	}

	const QString& LauncherProjectModel::ActiveProjectId() const
	{
		return m_activeProjectId;
	}

	void LauncherProjectModel::Refresh(const std::filesystem::path& repositoryRoot)
	{
		std::string errorMessage;
		const std::vector<SparkleProject> discoveredProjects = DiscoverProjects(repositoryRoot, errorMessage);

		QVector<LauncherProjectSummary> refreshedProjects;
		for (const SparkleProject& project : discoveredProjects)
		{
			refreshedProjects.push_back({QString::fromStdString(project.Id), QString::fromStdString(project.DisplayName), project.RootPath});
		}

		const QString nextActiveProjectId = ChooseInitialProjectId(refreshedProjects);

		const bool projectsChanged = !ProjectsMatch(m_projects, refreshedProjects);
		const bool activeProjectChanged = m_activeProjectId != nextActiveProjectId;
		m_projects = std::move(refreshedProjects);
		m_activeProjectId = nextActiveProjectId;

		if (projectsChanged || activeProjectChanged)
		{
			emit ProjectsChanged();
		}

		if (!errorMessage.empty())
		{
			emit ProjectDiscoveryFailed(QString::fromStdString(errorMessage));
		}
	}

	QString LauncherProjectModel::ChooseInitialProjectId(const QVector<LauncherProjectSummary>& projects)
	{
		for (const LauncherProjectSummary& project : projects)
		{
			if (project.Id == kDefaultProjectId)
			{
				return project.Id;
			}
		}

		return projects.empty() ? QString() : projects.front().Id;
	}
}
