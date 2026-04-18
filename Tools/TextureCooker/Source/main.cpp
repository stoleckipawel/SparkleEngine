#include "Cooking/TextureAssetCooker.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <format>
#include <iostream>
#include <objbase.h>
#include <string_view>

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
	std::vector<Engine::AssetAuthoring::TextureCookRequest> requests;
	std::string errorMessage;
	if (!Engine::AssetAuthoring::LoadTextureCookRequestList(requestFilePath, requests, errorMessage))
	{
		std::cerr << "TextureCooker: failed to inspect request file - " << errorMessage << "\n";
		return 5;
	}

	std::cout << "TextureCooker: request file='" << requestFilePath.string() << "' contains " << requests.size() << " texture request(s)\n";
	for (const Engine::AssetAuthoring::TextureCookRequest& request : requests)
	{
		std::cout << "  Texture '" << std::format("{:016X}", request.assetId) << "' colorSpace='"
		          << Engine::AssetAuthoring::GetTextureColorSpaceName(request.colorSpace) << "' output='"
		          << request.outputPath.string() << "' source='" << request.sourcePath.string() << "'\n";
	}

	return 0;
}

static int RunCookRequestFile(const std::filesystem::path& requestFilePath)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		std::cerr << "TextureCooker: failed to initialize COM for source texture loading\n";
		return 4;
	}

	std::vector<Engine::AssetAuthoring::TextureCookRequest> requests;
	std::string errorMessage;
	if (!Engine::AssetAuthoring::LoadTextureCookRequestList(requestFilePath, requests, errorMessage))
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		std::cerr << "TextureCooker: failed to load request file - " << errorMessage << "\n";
		return 6;
	}

	Engine::AssetAuthoring::TextureAssetCooker cooker;
	for (const Engine::AssetAuthoring::TextureCookRequest& request : requests)
	{
		if (!cooker.Cook(request, errorMessage))
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "TextureCooker: failed to cook texture '" << request.sourcePath.string() << "' - " << errorMessage << "\n";
			return 7;
		}
	}

	if (SUCCEEDED(coInitializeResult))
	{
		CoUninitialize();
	}

	std::cout << "TextureCooker: cooked " << requests.size() << " texture asset(s) from request file '" << requestFilePath.string() << "'\n";
	for (const Engine::AssetAuthoring::TextureCookRequest& request : requests)
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