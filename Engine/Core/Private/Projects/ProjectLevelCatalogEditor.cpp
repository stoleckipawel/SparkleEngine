#include "PCH.h"

#include "Projects/ProjectLevelCatalogEditor.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cstddef>
#include <sstream>
#include <utility>

bool ProjectLevelCatalogEditor::SetEntryBool(
    const std::filesystem::path& projectRoot,
    std::string_view sectionHeader,
    std::string_view entryId,
    std::string_view keyName,
    bool value,
    std::string& outErrorMessage)
{
	ProjectLevelCatalogEditor editor(
	    projectRoot / "Levels.catalog",
	    std::string(sectionHeader),
	    std::string(entryId),
	    std::string(keyName),
	    value);
	return editor.Apply(outErrorMessage);
}

ProjectLevelCatalogEditor::ProjectLevelCatalogEditor(
    std::filesystem::path catalogPath,
    std::string sectionHeader,
    std::string entryId,
    std::string keyName,
    bool value) :
    m_catalogPath(std::move(catalogPath)),
    m_sectionHeader(std::move(sectionHeader)),
    m_entryId(std::move(entryId)),
    m_keyName(std::move(keyName)),
    m_valueLine(m_keyName + " = " + (value ? "true" : "false"))
{
}

bool ProjectLevelCatalogEditor::Apply(std::string& outErrorMessage)
{
	if (!Read(outErrorMessage) || !UpdateEntry(outErrorMessage))
	{
		return false;
	}

	return Publish(outErrorMessage);
}

bool ProjectLevelCatalogEditor::Read(std::string& outErrorMessage)
{
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

bool ProjectLevelCatalogEditor::UpdateEntry(std::string& outErrorMessage)
{
	bool inRequestedSection = false;
	bool inTargetEntry = false;

	for (std::size_t index = 0; index < m_lines.size(); ++index)
	{
		const std::string trimmed = Strings::TrimCopy(m_lines[index]);
		if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';')
		{
			continue;
		}

		if (trimmed.front() == '[' && trimmed.back() == ']')
		{
			if (inTargetEntry)
			{
				m_lines.insert(
				    m_lines.begin() + static_cast<std::ptrdiff_t>(index),
				    m_valueLine);
				return true;
			}

			inRequestedSection = trimmed == m_sectionHeader;
			inTargetEntry = false;
			continue;
		}

		if (!inRequestedSection)
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
			inTargetEntry = Strings::UnquoteCopy(value) == m_entryId;
		}
		else if (inTargetEntry && key == m_keyName)
		{
			m_lines[index] = m_valueLine;
			return true;
		}
	}

	if (inTargetEntry)
	{
		m_lines.push_back(m_valueLine);
		return true;
	}

	outErrorMessage = "Project level catalog entry was not found: " + m_entryId;
	return false;
}

bool ProjectLevelCatalogEditor::Publish(std::string& outErrorMessage) const
{
	return Files::TryWriteAllTextAtomic(
	    m_catalogPath,
	    BuildText(),
	    outErrorMessage);
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
