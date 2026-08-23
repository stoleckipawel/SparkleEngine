#pragma once

#include <filesystem>

class ShaderCookCancellation final
{
public:
	ShaderCookCancellation() = delete;

	static bool IsRequested(const std::filesystem::path& signalPath);
	static void ThrowIfRequested(const std::filesystem::path& signalPath);
};
