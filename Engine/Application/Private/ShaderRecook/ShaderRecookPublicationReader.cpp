#include "PCH.h"

#include "ShaderRecook/ShaderRecookPublicationReader.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonReader.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

ShaderRecookPublicationReadResult ShaderRecookPublicationReader::Read(
    const std::filesystem::path& publicationPath) noexcept
{
	ShaderRecookPublicationReadResult result;
	std::error_code errorCode;
	if (!std::filesystem::exists(publicationPath, errorCode) || errorCode)
	{
		result.Missing = true;
		return result;
	}

	std::vector<std::uint8_t> bytes;
	std::string readErrorMessage;
	if (!Files::TryReadAllBytes(publicationPath, bytes, readErrorMessage))
	{
		result.Diagnostic =
		    "Shader recook publication could not be read; reload rejected before touching active packages. " +
		    readErrorMessage;
		return result;
	}

	const std::string text(bytes.begin(), bytes.end());
	if (text.empty())
	{
		result.Diagnostic =
		    "Shader recook publication is empty or partially written; reload rejected before touching active packages.";
		return result;
	}

	return Parse(text);
}

ShaderRecookPublicationReadResult ShaderRecookPublicationReader::Parse(std::string_view text) noexcept
{
	ShaderRecookPublicationReadResult result;
	std::string schema;
	std::string status;
	std::string registry;
	std::string registryHashText;
	std::uint64_t schemaVersion = 0;
	std::uint64_t publicationId = 0;
	std::uint64_t publishedAtUnixMs = 0;
	std::uint64_t registryHash = 0;
	if (!Json::TryReadStringProperty(text, "schema", schema) ||
	    !Json::TryReadUInt64Property(text, "schemaVersion", schemaVersion) ||
	    !Json::TryReadStringProperty(text, "status", status) ||
	    !Json::TryReadUInt64Property(text, "publicationId", publicationId) ||
	    !Json::TryReadUInt64Property(text, "publishedAtUnixMs", publishedAtUnixMs) ||
	    !Json::TryReadStringProperty(text, "registry", registry) ||
	    !Json::TryReadStringProperty(text, "registryHash", registryHashText) ||
	    !Json::TryParseHexUInt64(registryHashText, registryHash))
	{
		result.Diagnostic =
		    "Shader recook publication is invalid or partially written; reload rejected before touching active packages.";
		return result;
	}

	if (schema != "sparkle.shaderRecookResult" || schemaVersion != 1)
	{
		result.Diagnostic = "Shader recook publication has unsupported schema '" + schema + "' version " +
		                    std::to_string(schemaVersion) +
		                    "; reload rejected before touching active packages.";
		return result;
	}

	if (status != "succeeded")
	{
		result.Diagnostic = "Shader recook publication status is '" + status +
		                    "', not 'succeeded'; reload rejected before touching active packages.";
		return result;
	}

	result.Publication = ShaderRecookPublication {
	    .PublicationId = publicationId,
	    .PublishedAtUnixMs = publishedAtUnixMs,
	    .RegistryHash = registryHash,
	    .RegistryPath = std::filesystem::path(registry),
	    .Status = std::move(status)};
	return result;
}
