#include "NativeBuildOutputResetTests.h"

#include "NativeBuildOutputReset.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace SparkleLauncher::Tests
{
	class TemporaryWorkspace final
	{
	public:
		TemporaryWorkspace()
		{
			const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
			m_root = std::filesystem::temp_directory_path() / ("SparkleNativeBuildOutputResetTests-" + std::to_string(uniqueValue));
			std::filesystem::create_directories(m_root);
		}

		~TemporaryWorkspace() noexcept
		{
			std::error_code errorCode;
			std::filesystem::remove_all(m_root, errorCode);
		}

		TemporaryWorkspace(const TemporaryWorkspace&) = delete;
		TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

		const std::filesystem::path& Root() const { return m_root; }

		std::filesystem::path Write(std::string_view relativePath) const
		{
			const std::filesystem::path path = m_root / relativePath;
			std::filesystem::create_directories(path.parent_path());
			std::ofstream(path, std::ios::binary | std::ios::trunc) << "test\n";
			return path;
		}

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

	static void ResetClassificationIsNarrow()
	{
		Require(
		    RequiresNativeBuildOutputReset(BuildFilesFreshnessState::GeneratorMismatch),
		    "A generator or toolset mismatch did not require an output reset.");
		Require(
		    RequiresNativeBuildOutputReset(BuildFilesFreshnessState::FreshnessStampMismatch),
		    "A build-output contract mismatch did not require an output reset.");
		Require(
		    !RequiresNativeBuildOutputReset(BuildFilesFreshnessState::BuildInputChanged),
		    "An ordinary build-input change requested a destructive output reset.");
		Require(
		    !RequiresNativeBuildOutputReset(BuildFilesFreshnessState::BuildDirectoryMissing),
		    "A pristine workspace requested an unnecessary output reset.");
	}

	static void ResetPreservesSourcesAndCookedContent()
	{
		TemporaryWorkspace workspace;
		const std::filesystem::path buildDirectory = workspace.Root() / "build";
		const std::filesystem::path dependencySource = workspace.Write("build/_deps/library-src/source.cpp");
		const std::filesystem::path dependencyArchive = workspace.Write("build/_deps/library.zip");
		const std::filesystem::path cookedContent = workspace.Write("artifacts/dev/projects/TestProject/cooked/Scene.sparkmesh");
		const std::filesystem::path diagnostics = workspace.Write("artifacts/diagnostics/capture.txt");

		const std::filesystem::path cmakeCache = workspace.Write("build/CMakeCache.txt");
		const std::filesystem::path dependencyBuild = workspace.Write("build/_deps/library-build/object.obj");
		const std::filesystem::path dependencySubbuild = workspace.Write("build/_deps/library-subbuild/project.vcxproj");
		const std::filesystem::path libraryPdb =
		    workspace.Write("artifacts/dev/libraries/runtime-support/Renderer/DevelopmentEditor/Renderer.pdb");
		const std::filesystem::path launcherBinary = workspace.Write("artifacts/dev/launcher/DevelopmentEditor/SparkleLauncher.exe");
		const std::filesystem::path editorBinary =
		    workspace.Write("artifacts/dev/projects/TestProject/editor/DevelopmentEditor/TestEditor.exe");
		const std::filesystem::path runtimeBinary =
		    workspace.Write("artifacts/dev/projects/TestProject/runtime/DevelopmentGame/TestGame.exe");
		const std::filesystem::path symbols = workspace.Write("artifacts/symbols/runtime-support/Renderer/DevelopmentEditor/Renderer.pdb");

		std::string errorMessage;
		Require(
		    ResetNativeBuildOutputs(workspace.Root(), buildDirectory, errorMessage),
		    errorMessage.empty() ? "The incompatible output reset failed." : errorMessage);

		Require(std::filesystem::is_regular_file(dependencySource), "Downloaded dependency sources were removed.");
		Require(std::filesystem::is_regular_file(dependencyArchive), "A downloaded dependency archive was removed.");
		Require(std::filesystem::is_regular_file(cookedContent), "Cooked content was removed.");
		Require(std::filesystem::is_regular_file(diagnostics), "Unrelated diagnostics were removed.");
		Require(!std::filesystem::exists(cmakeCache), "Generated CMake state survived the reset.");
		Require(!std::filesystem::exists(dependencyBuild), "Dependency compiler outputs survived the reset.");
		Require(!std::filesystem::exists(dependencySubbuild), "Dependency subbuild state survived the reset.");
		Require(!std::filesystem::exists(libraryPdb), "A compiler-produced library PDB survived the reset.");
		Require(!std::filesystem::exists(launcherBinary), "A launcher binary survived the reset.");
		Require(!std::filesystem::exists(editorBinary), "An editor binary survived the reset.");
		Require(!std::filesystem::exists(runtimeBinary), "A runtime binary survived the reset.");
		Require(!std::filesystem::exists(symbols), "Compiler symbols survived the reset.");
	}

	static void ResetRejectsRepositoryAsBuildDirectory()
	{
		TemporaryWorkspace workspace;
		const std::filesystem::path repositoryFile = workspace.Write("CMakeLists.txt");

		std::string errorMessage;
		Require(
		    !ResetNativeBuildOutputs(workspace.Root(), workspace.Root(), errorMessage),
		    "The reset accepted the repository root as its build directory.");
		Require(std::filesystem::is_regular_file(repositoryFile), "The rejected reset removed repository content.");
		Require(!errorMessage.empty(), "The rejected reset did not explain its invalid scope.");
	}

	bool RunNativeBuildOutputResetTests(std::string& errorMessage)
	{
		try
		{
			ResetClassificationIsNarrow();
			ResetPreservesSourcesAndCookedContent();
			ResetRejectsRepositoryAsBuildDirectory();
			errorMessage.clear();
			return true;
		}
		catch (const std::exception& error)
		{
			errorMessage = error.what();
			return false;
		}
	}
}
