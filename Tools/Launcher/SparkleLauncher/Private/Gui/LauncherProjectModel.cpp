#include "LauncherProjectModel.h"

#include "SparkleLauncher/ProjectDiscovery.h"

namespace SparkleLauncher
{
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

		m_projects.clear();
		for (const SparkleProject& project : discoveredProjects)
		{
			m_projects.push_back({QString::fromStdString(project.Id), QString::fromStdString(project.DisplayName), project.RootPath});
		}

		const QString previousSelection = m_selectedProjectId;
		m_selectedProjectId = ChooseInitialProjectId(m_projects);

		emit ProjectsChanged();
		if (m_selectedProjectId != previousSelection)
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