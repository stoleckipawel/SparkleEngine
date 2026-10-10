#pragma once

#include "ToolWorkProgress.h"

#include <functional>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	class LauncherProgressStreamDecoder final
	{
	public:
		using OutputCallback = std::function<void(std::string_view)>;

		using ProgressCallback = std::function<void(const ToolWorkProgress&)>;

		void Consume(std::string_view output, const OutputCallback& outputCallback, const ProgressCallback& progressCallback);
		void Flush(const OutputCallback& outputCallback);

	private:
		bool Accept(const ToolWorkProgress& progress);

		std::string m_pendingLine;
		ToolWorkProgress m_lastProgress;
		bool m_hasProgress = false;
	};
}
