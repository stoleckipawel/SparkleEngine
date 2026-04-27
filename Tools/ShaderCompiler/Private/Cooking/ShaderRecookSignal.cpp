#include "PCH.h"

#include "Cooking/ShaderRecookSignal.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <chrono>
#include <format>

bool ShaderRecookSignal::Write(
    const std::filesystem::path& cacheDirectory,
    const std::filesystem::path& registryPath,
    ShaderRecookSignalResult& outResult,
    std::string& outErrorMessage)
{
	outResult = {};
	outResult.signalPath = Paths::ShaderRecookSignal(cacheDirectory);

	std::vector<std::uint8_t> registryBytes;
	if (!Files::TryReadAllBytes(registryPath, registryBytes, outErrorMessage))
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

	if (!Files::TryWriteAllTextAtomic(outResult.signalPath, contents, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}