#include "PCH.h"

#include "Projects/ProjectLevelCatalogEditor.h"
#include "Projects/ProjectLevelCatalogReader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cstddef>
#include <sstream>
#include <unordered_set>
#include <utility>

bool ProjectLevelCatalogEditor::SetLevelSelected(
    const std::filesystem::path& projectRoot,
    std::string_view levelId,
    bool selected,
    std::string& outErrorMessage)
{
	return SetLevelsSelected(projectRoot, {std::string(levelId)}, selected, outErrorMessage);
}

bool ProjectLevelCatalogEditor::SetLevelsSelected(
    const std::filesystem::path& projectRoot,
    const std::vector<std::string>& levelIds,
    bool selected,
    std::string& outErrorMessage)
{
	ProjectLevelCatalogEditor editor(projectRoot / "Levels.catalog", levelIds, selected);
	return editor.Apply(outErrorMessage);
}

ProjectLevelCatalogEditor::ProjectLevelCatalogEditor(std::filesystem::path catalogPath, std::vector<std::string> levelIds, bool selected) :
    m_catalogPath(std::move(catalogPath)),
    m_levelIds(std::move(levelIds)),
    m_valueLine(std::string("Selected = ") + (selected ? "true" : "false"))
{
}

bool ProjectLevelCatalogEditor::Apply(std::string& outErrorMessage)
{
	outErrorMessage.clear();
	if (!Read(outErrorMessage) || !UpdateEntries(outErrorMessage))
	{
		return false;
	}

	const std::string updatedText = BuildText();
	return Validate(updatedText, outErrorMessage) && Publish(updatedText, outErrorMessage);
}

bool ProjectLevelCatalogEditor::Read(std::string& outErrorMessage)
{
	m_lines.clear();
	std::string catalogText;
	if (!Files::TryReadAllText(m_catalogPath, catalogText, outErrorMessage))
	{
		return false;
	}

	std::istringstream input(catalogText);
	for (std::string line; std::getline(input, line);)
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		m_lines.push_back(std::move(line));
	}

	return true;
}

bool ProjectLevelCatalogEditor::UpdateEntries(std::string& outErrorMessage)
{
	std::unordered_set<std::string> remainingLevelIds(m_levelIds.begin(), m_levelIds.end());
	if (remainingLevelIds.empty())
	{
		return true;
	}

	bool inLevelSection = false;
	bool inTargetEntry = false;
	std::string currentLevelId;

	for (std::size_t index = 0; index < m_lines.size(); ++index)
	{
		const std::string trimmed = Strings::TrimCopy(m_lines[index]);
		if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';')
		{
			continue;
		}

		if (trimmed.front() == '[' && trimmed.back() == ']')
		{
			inLevelSection = trimmed == "[Level]";
			inTargetEntry = false;
			currentLevelId.clear();
			continue;
		}

		if (!inLevelSection)
		{
			continue;
		}

		std::string_view key;
		std::string_view value;
		if (!Strings::TrySplitKeyValue(trimmed, '=', key, value))
		{
			continue;
		}

		if (key == "Id")
		{
			currentLevelId = Strings::UnquoteCopy(value);
			inTargetEntry = remainingLevelIds.contains(currentLevelId);
		}
		else if (inTargetEntry && key == "Selected")
		{
			m_lines[index] = m_valueLine;
			remainingLevelIds.erase(currentLevelId);
			inTargetEntry = false;
		}
	}

	if (remainingLevelIds.empty())
	{
		return true;
	}

	outErrorMessage = "Project level catalog entry was not found: " + *remainingLevelIds.begin();
	return false;
}

bool ProjectLevelCatalogEditor::Validate(std::string_view text, std::string& outErrorMessage) const
{
	try
	{
		ProjectLevelCatalogReader::ValidateText(m_catalogPath.parent_path(), text);
		return true;
	}
	catch (const Diagnostics::Error& error)
	{
		outErrorMessage = std::string("Project level catalog update was rejected: ") + error.what();
		return false;
	}
}

bool ProjectLevelCatalogEditor::Publish(std::string_view text, std::string& outErrorMessage) const
{
	return Files::TryWriteAllTextAtomic(m_catalogPath, text, outErrorMessage);
}

std::string ProjectLevelCatalogEditor::BuildText() const
{
	std::string output;
	for (const std::string& line : m_lines)
	{
		output += line;
		output += '\n';
	}

	return output;
}
