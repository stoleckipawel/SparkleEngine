#include "SparkleLauncher/ContentDiscovery.h"

#include "Core/Public/FileSystemUtils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace SparkleLauncher::ContentDiscoveryTests
{
	class TemporaryRepository final
	{
	public:
		TemporaryRepository()
		{
			const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
			m_root = std::filesystem::temp_directory_path() / ("SparkleContentDiscoveryTests-" + std::to_string(uniqueValue));
			std::filesystem::create_directories(m_root / "Projects");
		}

		~TemporaryRepository() noexcept
		{
			std::error_code errorCode;
			std::filesystem::remove_all(m_root, errorCode);
		}

		TemporaryRepository(const TemporaryRepository&) = delete;
		TemporaryRepository& operator=(const TemporaryRepository&) = delete;

		const std::filesystem::path& Root() const { return m_root; }

		std::filesystem::path AddContent(std::string_view id) const
		{
			const std::filesystem::path contentRoot = m_root / "Projects" / id;
			std::filesystem::create_directories(contentRoot);
			std::ofstream(contentRoot / std::string(Filesystem::kProjectMarker), std::ios::binary | std::ios::trunc) << "test\n";
			return contentRoot;
		}

	private:
		std::filesystem::path m_root;
	};

	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	void ExactlyOneContentRootIsDiscovered()
	{
		TemporaryRepository repository;
		const std::filesystem::path expectedRoot = repository.AddContent("Example");

		std::string errorMessage;
		const std::optional<SparkleContent> content = DiscoverContentRoot(repository.Root(), errorMessage);

		Require(content.has_value(), "A repository with one marked content root was rejected.");
		Require(errorMessage.empty(), "Successful content discovery returned an error.");
		Require(content->Id == "Example", "Content discovery returned the wrong identifier.");
		Require(content->DisplayName == "Example", "Content discovery returned the wrong display name.");
		Require(content->RootPath == expectedRoot, "Content discovery returned the wrong root path.");
		Require(
		    content->MarkerPath == expectedRoot / std::string(Filesystem::kProjectMarker),
		    "Content discovery returned the wrong marker path.");
	}

	void MissingContentRootIsRejected()
	{
		TemporaryRepository repository;

		std::string errorMessage;
		const std::optional<SparkleContent> content = DiscoverContentRoot(repository.Root(), errorMessage);

		Require(!content.has_value(), "A repository without a marked content root was accepted.");
		Require(errorMessage.find("found 0") != std::string::npos, "Missing-content failure did not report the discovered count.");
	}

	void AmbiguousContentRootsAreRejected()
	{
		TemporaryRepository repository;
		repository.AddContent("First");
		repository.AddContent("Second");

		std::string errorMessage;
		const std::optional<SparkleContent> content = DiscoverContentRoot(repository.Root(), errorMessage);

		Require(!content.has_value(), "A repository with multiple marked content roots was accepted.");
		Require(errorMessage.find("found 2") != std::string::npos, "Ambiguous-content failure did not report the discovered count.");
	}
}

int main()
{
	try
	{
		SparkleLauncher::ContentDiscoveryTests::ExactlyOneContentRootIsDiscovered();
		SparkleLauncher::ContentDiscoveryTests::MissingContentRootIsRejected();
		SparkleLauncher::ContentDiscoveryTests::AmbiguousContentRootsAreRejected();
		std::cout << "Content discovery tests passed.\n";
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Content discovery tests failed: " << exception.what() << '\n';
		return 1;
	}
}
