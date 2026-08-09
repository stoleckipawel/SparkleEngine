#include "Api/AssetCookerService.h"
#include "Discovery/AssetCookerDiscovery.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace AssetCookerProfileContractTests
{
	class TemporaryRepository final
	{
	public:
		TemporaryRepository()
		{
			const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
			m_root = std::filesystem::temp_directory_path() / ("SparkleAssetCookerProfileTests-" + std::to_string(uniqueValue));
			std::filesystem::create_directories(m_root / "Projects" / "TestProject");
			std::ofstream(m_root / "Projects" / "TestProject" / ".sparkle-project") << "test\n";
		}

		~TemporaryRepository() noexcept
		{
			std::error_code errorCode;
			std::filesystem::remove_all(m_root, errorCode);
		}

		TemporaryRepository(const TemporaryRepository&) = delete;
		TemporaryRepository& operator=(const TemporaryRepository&) = delete;

		const std::filesystem::path& Root() const { return m_root; }

	private:
		std::filesystem::path m_root;
	};

	static void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	static void RuntimeProfilesResolveToEditorHostProfiles()
	{
		Require(AssetCookerDiscovery::ResolveToolProfile("DebugEditor") == "DebugEditor", "DebugEditor host profile changed.");
		Require(AssetCookerDiscovery::ResolveToolProfile("DebugGame") == "DebugEditor", "DebugGame did not use DebugEditor tools.");
		Require(
		    AssetCookerDiscovery::ResolveToolProfile("DevelopmentEditor") == "DevelopmentEditor",
		    "DevelopmentEditor host profile changed.");
		Require(
		    AssetCookerDiscovery::ResolveToolProfile("DevelopmentGame") == "DevelopmentEditor",
		    "DevelopmentGame did not use DevelopmentEditor tools.");
		Require(AssetCookerDiscovery::ResolveToolProfile("ShippingEditor") == "ShippingEditor", "ShippingEditor host profile changed.");
		Require(
		    AssetCookerDiscovery::ResolveToolProfile("ShippingGame") == "ShippingEditor",
		    "ShippingGame did not use ShippingEditor tools.");
		Require(!AssetCookerDiscovery::ResolveToolProfile("Release").has_value(), "An unsupported profile was accepted.");
	}

	static void MismatchedHostProfileIsRejected()
	{
		AssetCookerService service("C:/Sparkle", "TestProject", "DevelopmentGame", "DevelopmentGame");
		const AssetCookerServiceResult result = service.Cook("TestProject", "DevelopmentGame", AssetCookerCategory_Shaders);

		Require(result.exitCode != 0, "A runtime profile was accepted as a host-tool profile.");
		Require(!result.diagnostics.empty(), "A mismatched host profile failed without diagnostics.");
		Require(
		    result.diagnostics.front().message.find("does not match runtime profile") != std::string::npos,
		    "A mismatched host profile did not report the profile contract.");
	}

	static void MissingToolReportsTheAuthoritativeArtifactPath()
	{
		TemporaryRepository repository;
		const std::string repositoryRoot = repository.Root().string();
		AssetCookerService service(repositoryRoot.c_str(), "TestProject", "DevelopmentGame", "DevelopmentEditor");
		const AssetCookerServiceResult result = service.Cook("TestProject", "DevelopmentGame", AssetCookerCategory_Shaders);
		const std::filesystem::path expected =
		    repository.Root() / "artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe";

		Require(result.exitCode != 0, "A cook with a missing ShaderCompiler unexpectedly succeeded.");
		Require(!result.diagnostics.empty(), "A missing ShaderCompiler failed without diagnostics.");
		Require(
		    std::filesystem::path(result.diagnostics.front().sourcePath) == expected,
		    "The missing tool used a non-authoritative path.");
		Require(result.diagnostics.front().sourcePath.find("build/bin") == std::string::npos, "The removed build/bin fallback returned.");
	}
}

int main()
{
	using namespace AssetCookerProfileContractTests;
	try
	{
		RuntimeProfilesResolveToEditorHostProfiles();
		MismatchedHostProfileIsRejected();
		MissingToolReportsTheAuthoritativeArtifactPath();
		std::cout << "[PASS] AssetCooker profile and artifact contract\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "[FAIL] AssetCooker profile and artifact contract: " << error.what() << '\n';
		return 1;
	}
}
