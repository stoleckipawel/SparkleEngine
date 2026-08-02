#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ProjectLevelCatalogTests
{
	class TemporaryProject final
	{
	public:
		TemporaryProject()
		{
			const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
			m_root = std::filesystem::temp_directory_path() / ("SparkleProjectLevelCatalogTests-" + std::to_string(uniqueValue));
			std::filesystem::create_directories(m_root);
		}

		~TemporaryProject() noexcept
		{
			std::error_code error;
			std::filesystem::remove_all(m_root, error);
		}

		TemporaryProject(const TemporaryProject&) = delete;
		TemporaryProject& operator=(const TemporaryProject&) = delete;

		const std::filesystem::path& Root() const noexcept { return m_root; }

		void Write(std::string_view relativePath, std::string_view text) const
		{
			const std::filesystem::path path = m_root / relativePath;
			std::filesystem::create_directories(path.parent_path());
			std::ofstream output(path, std::ios::binary | std::ios::trunc);
			if (!output || !output.write(text.data(), static_cast<std::streamsize>(text.size())))
			{
				throw std::runtime_error("Could not write test input: " + path.string());
			}
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

	void RequireCatalogError(const TemporaryProject& project, std::string_view catalogText, std::string_view expectedMessage)
	{
		project.Write("Levels.catalog", catalogText);
		try
		{
			ProjectLevelCatalogFile::Load(project.Root());
		}
		catch (const Diagnostics::Error& error)
		{
			Require(std::string_view(error.what()).find(expectedMessage) != std::string_view::npos, error.what());
			return;
		}

		throw std::runtime_error("Invalid catalog was accepted.");
	}

	void ValidCatalogSupportsAtomicSelectionUpdate()
	{
		TemporaryProject project;
		project.Write("Levels/Empty.level", "Name = Empty\n");
		project.Write("Levels/Example.level", "Name = Example\n");
		project.Write(
		    "Levels.catalog",
		    "[Level]\n"
		    "Id = Empty\n"
		    "Source = Levels/Empty.level\n"
		    "Selected = false\n"
		    "\n"
		    "[Level]\n"
		    "Id = Example\n"
		    "Source = Levels/Example.level\n"
		    "SourcePage = https://example.invalid/level\n"
		    "Selected = false\n");

		ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(project.Root());
		Require(catalog.levels.size() == 2, "Valid catalog did not produce both levels.");
		Require(!catalog.levels.front().selected, "Level selection default was not preserved.");
		Require(catalog.IsLevelReady(catalog.levels.front()), "Existing repository level was not ready.");
		Require(catalog.levels.back().sourcePageUrl == "https://example.invalid/level", "Level source page was not parsed.");

		std::string errorMessage;
		Require(
		    ProjectLevelCatalogFile::SetLevelsSelected(project.Root(), {"Empty", "Example"}, true, errorMessage),
		    errorMessage.empty() ? "Batch selection update failed without a diagnostic." : errorMessage);
		catalog = ProjectLevelCatalogFile::Load(project.Root());
		Require(catalog.levels.front().selected && catalog.levels.back().selected, "Batch selection update was not published atomically.");
	}

	void TraversalIsRejected()
	{
		TemporaryProject project;
		RequireCatalogError(
		    project,
		    "[Level]\nId = Escape\nSource = ../Escape.level\nSelected = false\n",
		    "must remain below the project root");
	}

	void MissingSelectionIsRejected()
	{
		TemporaryProject project;
		RequireCatalogError(project, "[Level]\nId = Empty\nSource = Levels/Empty.level\n", "missing required field 'Selected'");
	}

	void RetiredSyncedFieldIsRejected()
	{
		TemporaryProject project;
		RequireCatalogError(
		    project,
		    "[Level]\nId = Empty\nSource = Levels/Empty.level\nSynced = true\n",
		    "Unsupported level catalog field 'Synced'");
	}

	void InvalidArchiveDigestIsRejected()
	{
		TemporaryProject project;
		RequireCatalogError(
		    project,
		    "[Level]\n"
		    "Id = Empty\n"
		    "Source = Levels/Empty.level\n"
		    "Selected = false\n"
		    "\n"
		    "[AssetPack]\n"
		    "Id = Example\n"
		    "DisplayName = Example\n"
		    "Root = Assets/Example\n"
		    "ExtractRoot = Assets/Example\n"
		    "Required = scene.gltf\n"
		    "SourceUrl = https://example.invalid/example.zip\n"
		    "SourcePage = https://example.invalid/\n"
		    "Archive = example.zip\n"
		    "ArchiveBytes = 1\n"
		    "ArchiveSha256 = invalid\n"
		    "Version = 1\n"
		    "License = Test only\n"
		    "External = true\n"
		    "DownloadSupported = true\n"
		    "RuntimeSupported = true\n",
		    "invalid SHA-256 digest");
	}

	void UnsafeAssetPackLeafPathsAreRejected()
	{
		TemporaryProject project;
		const std::string prefix = "[Level]\n"
		                           "Id = Empty\n"
		                           "Source = Levels/Empty.level\n"
		                           "Selected = false\n"
		                           "\n"
		                           "[AssetPack]\n"
		                           "Id = Example\n"
		                           "DisplayName = Example\n"
		                           "Root = Assets/Example\n"
		                           "ExtractRoot = Assets/Example\n";
		const std::string suffix = "SourceUrl = https://example.invalid/example.zip\n"
		                           "SourcePage = https://example.invalid/\n"
		                           "ArchiveBytes = 1\n"
		                           "ArchiveSha256 = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
		                           "Version = 1\n"
		                           "License = Test only\n"
		                           "External = true\n"
		                           "DownloadSupported = true\n"
		                           "RuntimeSupported = true\n";

		RequireCatalogError(project, prefix + "Required = .\nArchive = example.zip\n" + suffix, "unsafe required path");
		RequireCatalogError(project, prefix + "Required = scene.gltf\nArchive = ..\n" + suffix, "must not contain a path");
#if defined(_WIN32)
		RequireCatalogError(project, prefix + "Required = C:escape.asset\nArchive = example.zip\n" + suffix, "unsafe required path");
#endif
	}

	void OverlappingPublicationRootsAreRejected()
	{
		TemporaryProject project;
		RequireCatalogError(
		    project,
		    "[Level]\n"
		    "Id = Empty\n"
		    "Source = Levels/Empty.level\n"
		    "Selected = false\n"
		    "\n"
		    "[AssetPack]\n"
		    "Id = ParentRoot\n"
		    "DisplayName = Parent Root\n"
		    "Root = Assets/Parent\n"
		    "ExtractRoot = Assets/Parent\n"
		    "Required = parent.gltf\n"
		    "SourceUrl = https://example.invalid/parent.zip\n"
		    "SourcePage = https://example.invalid/\n"
		    "Archive = parent.zip\n"
		    "ArchiveBytes = 1\n"
		    "ArchiveSha256 = aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
		    "Version = 1\n"
		    "License = Test only\n"
		    "External = true\n"
		    "DownloadSupported = true\n"
		    "RuntimeSupported = true\n"
		    "\n"
		    "[AssetPack]\n"
		    "Id = ChildRoot\n"
		    "DisplayName = Child Root\n"
		    "Root = Assets/Parent/Child\n"
		    "ExtractRoot = Assets/Parent/Child\n"
		    "Required = child.gltf\n"
		    "SourceUrl = https://example.invalid/child.zip\n"
		    "SourcePage = https://example.invalid/\n"
		    "Archive = child.zip\n"
		    "ArchiveBytes = 1\n"
		    "ArchiveSha256 = bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
		    "Version = 1\n"
		    "License = Test only\n"
		    "External = true\n"
		    "DownloadSupported = true\n"
		    "RuntimeSupported = true\n",
		    "overlapping extraction roots");
	}

	void SupportedPackCannotDependOnUnsupportedParent()
	{
		TemporaryProject project;
		RequireCatalogError(
		    project,
		    "[Level]\n"
		    "Id = Empty\n"
		    "Source = Levels/Empty.level\n"
		    "Selected = false\n"
		    "\n"
		    "[AssetPack]\n"
		    "Id = FutureBase\n"
		    "DisplayName = Future Base\n"
		    "Root = Assets/FutureBase\n"
		    "Required = base.asset\n"
		    "RuntimeBlocker = Runtime capability is unavailable.\n"
		    "External = false\n"
		    "DownloadSupported = false\n"
		    "RuntimeSupported = false\n"
		    "\n"
		    "[AssetPack]\n"
		    "Id = SupportedChild\n"
		    "DisplayName = Supported Child\n"
		    "Root = Assets/SupportedChild\n"
		    "Required = child.asset\n"
		    "Parent = FutureBase\n"
		    "External = false\n"
		    "DownloadSupported = false\n"
		    "RuntimeSupported = true\n",
		    "depends on runtime-unsupported parent");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	using namespace ProjectLevelCatalogTests;
	int failureCount = 0;
	failureCount += Run("valid catalog and selection update", ValidCatalogSupportsAtomicSelectionUpdate);
	failureCount += Run("path traversal rejection", TraversalIsRejected);
	failureCount += Run("required selection field", MissingSelectionIsRejected);
	failureCount += Run("retired Synced field", RetiredSyncedFieldIsRejected);
	failureCount += Run("archive digest validation", InvalidArchiveDigestIsRejected);
	failureCount += Run("safe asset-pack leaf paths", UnsafeAssetPackLeafPathsAreRejected);
	failureCount += Run("exclusive publication roots", OverlappingPublicationRootsAreRejected);
	failureCount += Run("runtime support dependency", SupportedPackCannotDependOnUnsupportedParent);
	return failureCount == 0 ? 0 : 1;
}
