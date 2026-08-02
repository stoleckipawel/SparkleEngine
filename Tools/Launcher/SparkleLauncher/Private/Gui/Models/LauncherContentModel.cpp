#include "LauncherContentModel.h"

#include "SparkleLauncher/ContentDiscovery.h"

#include <string>

namespace SparkleLauncher
{
	static bool ContentMatches(const LauncherContentSummary& left, const LauncherContentSummary& right)
	{
		return left.Id == right.Id && left.DisplayName == right.DisplayName && left.RootPath == right.RootPath;
	}

	LauncherContentModel::LauncherContentModel(QObject* parent) :
	    QObject(parent)
	{
	}

	const QString& LauncherContentModel::ContentId() const
	{
		return m_content.Id;
	}

	const LauncherContentSummary* LauncherContentModel::Content() const
	{
		return m_hasContent ? &m_content : nullptr;
	}

	void LauncherContentModel::Refresh(const std::filesystem::path& repositoryRoot)
	{
		std::string errorMessage;
		const std::optional<SparkleContent> discoveredContent = DiscoverContentRoot(repositoryRoot, errorMessage);

		LauncherContentSummary refreshedContent;
		const bool hasRefreshedContent = discoveredContent.has_value();
		if (hasRefreshedContent)
		{
			const SparkleContent& content = *discoveredContent;
			refreshedContent = {QString::fromStdString(content.Id), QString::fromStdString(content.DisplayName), content.RootPath};
		}

		const bool contentChanged = m_hasContent != hasRefreshedContent || (m_hasContent && !ContentMatches(m_content, refreshedContent));
		m_content = std::move(refreshedContent);
		m_hasContent = hasRefreshedContent;

		if (contentChanged)
		{
			emit ContentChanged();
		}

		if (!discoveredContent.has_value() && errorMessage.empty())
		{
			errorMessage = "The repository content root could not be discovered.";
		}
		if (!errorMessage.empty())
		{
			emit ContentDiscoveryFailed(QString::fromStdString(errorMessage));
		}
	}
}
