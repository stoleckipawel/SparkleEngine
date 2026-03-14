#pragma once

namespace Logging
{
	class Buffer;

	void WriteToSinks(Buffer& buffer) noexcept;
	void AppendPlatformError(Buffer& buffer, long hr) noexcept;
	void BreakInDebuggerIfAttached() noexcept;
}  // namespace Logging