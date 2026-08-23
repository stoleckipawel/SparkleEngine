#include "PCH.h"

#include "Cooking/ShaderCookCancellation.h"

#include "Core/Public/Diagnostics/Error.h"

#include <format>

bool ShaderCookCancellation::IsRequested(const std::filesystem::path& signalPath)
{
	if (signalPath.empty())
	{
		return false;
	}

	std::error_code errorCode;
	const bool requested = std::filesystem::is_regular_file(signalPath, errorCode);
	if (errorCode)
	{
		throw Diagnostics::Error(
		    std::format("Failed to inspect shader cook cancellation signal '{}': {}", signalPath.string(), errorCode.message()));
	}
	return requested;
}

void ShaderCookCancellation::ThrowIfRequested(const std::filesystem::path& signalPath)
{
	if (IsRequested(signalPath))
	{
		throw Diagnostics::Error("Shader cook was cancelled before publication.");
	}
}
