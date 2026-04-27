#include "Cooking/TextureAssetCooker.h"
#include "CookArtifactCache.h"
#include "D3D12/Textures/CookedTextureAsset.h"
#include "TextureCookRequestList.h"

#include "Core/Public/Hash/HashUtils.h"

#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <objbase.h>
#include <string_view>

static constexpr std::uint32_t kTextureCookerCookerVersion = 1;

static bool IsInspectRequestFileCommand(std::string_view command) noexcept
{
	return command == "inspect-request-file" || command == "inspect";
}

static bool IsCookRequestFileCommand(std::string_view command) noexcept
{
	return command == "cook-request-file" || command == "cook";
}

static int RunInspectRequestFile(const std::filesystem::path& requestFilePath)
{
	std::vector<AssetAuthoring::TextureCookRequest> requests;
	std::string errorMessage;
	if (!AssetAuthoring::LoadTextureCookRequestList(requestFilePath, requests, errorMessage))
	{
		std::cerr << "TextureCooker: failed to inspect request file - " << errorMessage << "\n";
		return 5;
	}

	std::cout << "TextureCooker: request file='" << requestFilePath.string() << "' contains " << requests.size() << " texture request(s)\n";
	for (const AssetAuthoring::TextureCookRequest& request : requests)
	{
		std::cout << "  Texture '" << std::format("{:016X}", request.assetId) << "' colorSpace='"
		          << AssetAuthoring::GetTextureColorSpaceName(request.colorSpace) << "' output='"
		          << request.outputPath.string() << "' source='" << request.sourcePath.string() << "'\n";
	}

	return 0;
}

static bool BuildTextureCookArtifactKey(
    const AssetAuthoring::TextureCookRequest& request,
    Cook::CookArtifactKey& outKey,
    std::string& outErrorMessage)
{
	std::uint64_t sourceHash = 0;
	if (!Hash::TryFnv1a64File(request.sourcePath, sourceHash, outErrorMessage))
	{
		return false;
	}

	outKey = Cook::CookArtifactKey{
	    .assetType = "Texture",
	    .assetId = std::format("{:016X}", request.assetId),
	    .cookerName = "TextureCooker",
	    .outputPath = request.outputPath,
	    .cookedFormatVersion = kCookedTextureAssetVersion,
	    .cookerVersion = kTextureCookerCookerVersion,
	    .sourceHash = sourceHash,
	    .dependencyHash = 0,
	    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash(
	        std::string("ColorSpace=") + AssetAuthoring::GetTextureColorSpaceName(request.colorSpace))};
	outErrorMessage.clear();
	return true;
}

static int RunCookRequestFile(const std::filesystem::path& requestFilePath)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		std::cerr << "TextureCooker: failed to initialize COM for source texture loading\n";
		return 4;
	}

	std::vector<AssetAuthoring::TextureCookRequest> requests;
	std::string errorMessage;
	if (!AssetAuthoring::LoadTextureCookRequestList(requestFilePath, requests, errorMessage))
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		std::cerr << "TextureCooker: failed to load request file - " << errorMessage << "\n";
		return 6;
	}

	AssetAuthoring::TextureAssetCooker cooker;
	std::size_t cookedCount = 0;
	std::size_t skippedCount = 0;
	for (const AssetAuthoring::TextureCookRequest& request : requests)
	{
		Cook::CookArtifactKey artifactKey;
		if (!BuildTextureCookArtifactKey(request, artifactKey, errorMessage))
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "TextureCooker: failed to build cook identity for texture '" << request.sourcePath.string() << "' - "
			          << errorMessage << "\n";
			return 7;
		}

		if (Cook::CookArtifactCache::IsCurrent(artifactKey, errorMessage))
		{
			++skippedCount;
			continue;
		}

		if (!errorMessage.empty())
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "TextureCooker: failed to inspect cooked texture metadata for '" << request.sourcePath.string() << "' - "
			          << errorMessage << "\n";
			return 7;
		}

		if (!cooker.Cook(request, errorMessage))
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "TextureCooker: failed to cook texture '" << request.sourcePath.string() << "' - " << errorMessage << "\n";
			return 7;
		}

		if (!Cook::CookArtifactCache::Publish(artifactKey, errorMessage))
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "TextureCooker: failed to publish cook metadata for texture '" << request.sourcePath.string() << "' - "
			          << errorMessage << "\n";
			return 7;
		}

		++cookedCount;
	}

	if (SUCCEEDED(coInitializeResult))
	{
		CoUninitialize();
	}

	std::cout << "TextureCooker: processed " << requests.size() << " texture asset(s) from request file '" << requestFilePath.string()
	          << "'; cooked=" << cookedCount << ", skipped=" << skippedCount << "\n";
	for (const AssetAuthoring::TextureCookRequest& request : requests)
	{
		std::cout << "  Texture '" << std::format("{:016X}", request.assetId) << "' output='" << request.outputPath.string()
		          << "'\n";
	}

	return 0;
}

int main(int argc, char** argv)
{
	if (argc == 3)
	{
		const std::string_view command(argv[1]);
		if (IsInspectRequestFileCommand(command))
		{
			return RunInspectRequestFile(std::filesystem::path(argv[2]));
		}

		if (IsCookRequestFileCommand(command))
		{
			return RunCookRequestFile(std::filesystem::path(argv[2]));
		}
	}

	std::cerr << "Usage:\n"
	          << "  TextureCooker inspect-request-file <request-file-path>\n"
	          << "  TextureCooker cook-request-file <request-file-path>\n"
	          << "\n"
	          << "Compatibility:\n"
	          << "  TextureCooker inspect <request-file-path>\n"
	          << "  TextureCooker cook <request-file-path>\n";
	return 1;
}