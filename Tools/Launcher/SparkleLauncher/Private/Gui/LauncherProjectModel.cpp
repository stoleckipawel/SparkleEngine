#include "LauncherProjectModel.h"

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

	const QString& LauncherProjectModel::SelectedProjectId() const
	{
		return m_selectedProjectId;
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

		const QString previousSelection = m_selectedProjectId;
		QString nextSelection = previousSelection;
		bool selectionStillExists = false;
		for (const LauncherProjectSummary& project : refreshedProjects)
		{
			if (project.Id == previousSelection)
			{
				selectionStillExists = true;
				break;
			}
		}
		if (!selectionStillExists)
		{
			nextSelection = ChooseInitialProjectId(refreshedProjects);
		}

		const bool projectsChanged = !ProjectsMatch(m_projects, refreshedProjects);
		const bool selectionChanged = m_selectedProjectId != nextSelection;
		m_projects = std::move(refreshedProjects);
		m_selectedProjectId = nextSelection;

		if (projectsChanged)
		{
			emit ProjectsChanged();
		}
		if (selectionChanged)
		{
			emit SelectionChanged(m_selectedProjectId);
		}

		if (!errorMessage.empty())
		{
			emit ProjectDiscoveryFailed(QString::fromStdString(errorMessage));
		}
	}

	void LauncherProjectModel::SelectProject(const QString& projectId)
	{
		if (m_selectedProjectId == projectId)
		{
			return;
		}

		for (const LauncherProjectSummary& project : m_projects)
		{
			if (project.Id == projectId)
			{
				m_selectedProjectId = projectId;
				emit SelectionChanged(m_selectedProjectId);
				return;
			}
		}
	}

	QString LauncherProjectModel::ChooseInitialProjectId(const QVector<LauncherProjectSummary>& projects)
	{
		for (const LauncherProjectSummary& project : projects)
		{
			if (project.Id == "Showcase")
			{
				return project.Id;
			}
		}

		return projects.empty() ? QString() : projects.front().Id;
	}
}
