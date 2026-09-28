#include "LauncherProgressStreamDecoder.h"

#include "ToolWorkProgress.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	static constexpr std::size_t kMaximumProgressLineBytes = 4096;

	void LauncherProgressStreamDecoder::Consume(std::string_view output, const ProgressCallback& progressCallback)
	{
		m_pendingLine.append(output);
		for (std::size_t newline = m_pendingLine.find('\n'); newline != std::string::npos; newline = m_pendingLine.find('\n'))
		{
			const std::string_view line(m_pendingLine.data(), newline);
			const std::optional<ToolWorkProgress> progress = ToolWorkProgressProtocol::Parse(line);
			if (progress && Accept(*progress) && progressCallback)
			{
				progressCallback(*progress);
			}
			m_pendingLine.erase(0, newline + 1);
		}

		if (m_pendingLine.size() > kMaximumProgressLineBytes)
		{
			m_pendingLine.clear();
		}
	}

	bool LauncherProgressStreamDecoder::Accept(const ToolWorkProgress& progress)
	{
		if (progress.phase.empty())
		{
			return false;
		}
		if (m_hasProgress && progress.phase == m_lastProgress.phase)
		{
			if (progress.total != m_lastProgress.total || progress.completed < m_lastProgress.completed)
			{
				return false;
			}
			if (progress.completed == m_lastProgress.completed)
			{
				return false;
			}
		}

		m_lastProgress = progress;
		m_hasProgress = true;
		return true;
	}
}
