#include "PCH.h"

#include "Cooking/ShaderRecookSignal.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"

#include <chrono>
#include <format>

std::filesystem::path ShaderRecookSignal::BuildPath(const std::filesystem::path& cacheDirectory)
{
	return cacheDirectory / "recook.signal";
}

bool ShaderRecookSignal::Write(
    const std::filesystem::path& cacheDirectory,
    const std::filesystem::path& registryPath,
    ShaderRecookSignalResult& outResult,
    std::string& outErrorMessage)
{
	outResult = {};
	outResult.signalPath = BuildPath(cacheDirectory);

	std::vector<std::uint8_t> registryBytes;
	if (!Engine::Files::TryReadAllBytes(registryPath, registryBytes, outErrorMessage))
	{
		outErrorMessage = "Failed to read shader registry for recook signal - " + outErrorMessage;
		return false;
	}

	outResult.registryHash = Hash::Fnv1a64(registryBytes.data(), registryBytes.size());
	const auto now = std::chrono::system_clock::now().time_since_epoch();
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	const std::string contents = std::format(
	    "registry={}\nregistryHash={:016X}\ntimestampMs={}\n",
	    registryPath.generic_string(),
	    outResult.registryHash,
	    milliseconds);

	const std::filesystem::path tempPath = outResult.signalPath.string() + ".tmp";
	if (!Engine::Files::TryWriteAllText(tempPath, contents, outErrorMessage))
	{
		return false;
	}

	std::error_code ec;
	std::filesystem::remove(outResult.signalPath, ec);
	ec.clear();
	std::filesystem::rename(tempPath, outResult.signalPath, ec);
	if (ec)
	{
		outErrorMessage = "Failed to publish shader recook signal '" + outResult.signalPath.string() + "' - " + ec.message();
		return false;
	}

	outErrorMessage.clear();
	return true;
}