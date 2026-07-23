#include "PCH.h"

#include "Core/Public/Strings/StringTableBuilder.h"

namespace Strings
{
	StringTableEntry StringTableBuilder::Add(std::string_view value)
	{
		if (value.empty())
		{
			return {};
		}

		const std::uint32_t offset = static_cast<std::uint32_t>(m_bytes.size());
		m_bytes.insert(m_bytes.end(), value.begin(), value.end());
		return StringTableEntry{offset, static_cast<std::uint32_t>(value.size())};
	}
}
