#include "PCH.h"

#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <fstream>
#include <unordered_set>
#include <vector>

namespace Files
{
	class FileSetPublisher final
	{
	  public:
		bool Publish(std::span<const FilePublication> files, std::string& outErrorMessage)
		{
			if (!BuildPublicationPlan(files, outErrorMessage))
			{
				return false;
			}
			if (!PreserveAndPublish(outErrorMessage))
			{
				Rollback();
				return false;
			}

			DiscardBackups();
			outErrorMessage.clear();
			return true;
		}

	  private:
		struct PublicationState final
		{
			FilePublication File;
			std::filesystem::path BackupPath;
			bool HadPublishedFile = false;
			bool Published = false;
		};

		bool BuildPublicationPlan(std::span<const FilePublication> files, std::string& outErrorMessage)
		{
			m_states.reserve(files.size());
			std::unordered_set<std::string> destinations;
			for (const FilePublication& file : files)
			{
				std::error_code errorCode;
				if (file.StagedPath.empty() || file.PublishedPath.empty() ||
				    !std::filesystem::is_regular_file(file.StagedPath, errorCode) || errorCode)
				{
					outErrorMessage = std::format("Publication input '{}' is missing or is not a file", file.StagedPath.string());
					return false;
				}

				const std::string destination = file.PublishedPath.lexically_normal().generic_string();
				if (!destinations.insert(destination).second)
				{
					outErrorMessage = std::format("Publication destination '{}' appears more than once", file.PublishedPath.string());
					return false;
				}

				PublicationState& state = m_states.emplace_back();
				state.File = file;
				state.BackupPath = BuildTemporaryPath(file.PublishedPath, ".previous-generation");
			}
			return true;
		}

		bool PreserveAndPublish(std::string& outErrorMessage)
		{
			for (PublicationState& state : m_states)
			{
				std::error_code errorCode;
				std::filesystem::remove(state.BackupPath, errorCode);
				errorCode.clear();
				state.HadPublishedFile = std::filesystem::exists(state.File.PublishedPath, errorCode) && !errorCode;
				if (state.HadPublishedFile)
				{
					std::filesystem::rename(state.File.PublishedPath, state.BackupPath, errorCode);
					if (errorCode)
					{
						outErrorMessage =
						    std::format("Failed to preserve active output '{}' before publication", state.File.PublishedPath.string());
						return false;
					}
				}

				errorCode.clear();
				std::filesystem::rename(state.File.StagedPath, state.File.PublishedPath, errorCode);
				if (errorCode)
				{
					outErrorMessage = std::format(
					    "Failed to publish staged output '{}' as '{}'",
					    state.File.StagedPath.string(),
					    state.File.PublishedPath.string());
					return false;
				}
				state.Published = true;
			}
			return true;
		}

		void Rollback() noexcept
		{
			for (auto state = m_states.rbegin(); state != m_states.rend(); ++state)
			{
				std::error_code errorCode;
				if (state->Published)
				{
					std::filesystem::remove(state->File.PublishedPath, errorCode);
				}
				if (state->HadPublishedFile)
				{
					errorCode.clear();
					std::filesystem::rename(state->BackupPath, state->File.PublishedPath, errorCode);
				}
			}
		}

		void DiscardBackups() noexcept
		{
			for (const PublicationState& state : m_states)
			{
				std::error_code errorCode;
				std::filesystem::remove(state.BackupPath, errorCode);
			}
		}

		std::vector<PublicationState> m_states;
	};

	std::filesystem::path BuildTemporaryPath(const std::filesystem::path& path, std::string_view suffix)
	{
		std::filesystem::path temporaryPath = path;
		temporaryPath += std::string(suffix.empty() ? std::string_view{".tmp"} : suffix);
		return temporaryPath;
	}

	bool TryFinalizeTemporaryFile(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage)
	{
		std::error_code errorCode;
		std::filesystem::rename(temporaryPath, finalPath, errorCode);
		if (!errorCode)
		{
			outErrorMessage.clear();
			return true;
		}

		if (std::filesystem::exists(finalPath))
		{
			errorCode.clear();
			std::filesystem::remove(finalPath, errorCode);
			if (errorCode)
			{
				outErrorMessage =
				    std::format("Failed to replace existing output '{}'. The destination file may still be in use.", finalPath.string());
				return false;
			}

			errorCode.clear();
			std::filesystem::rename(temporaryPath, finalPath, errorCode);
			if (!errorCode)
			{
				outErrorMessage.clear();
				return true;
			}
		}

		outErrorMessage = std::format("Failed to move temporary file '{}' into place as '{}'", temporaryPath.string(), finalPath.string());
		return false;
	}

	bool TryFinalizeTemporaryFileIfMissing(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage)
	{
		std::error_code errorCode;
		if (std::filesystem::exists(finalPath, errorCode) && !errorCode)
		{
			CleanupTemporaryFile(temporaryPath);
			outErrorMessage.clear();
			return true;
		}

		errorCode.clear();
		std::filesystem::rename(temporaryPath, finalPath, errorCode);
		if (!errorCode)
		{
			outErrorMessage.clear();
			return true;
		}

		std::error_code existsError;
		if (std::filesystem::exists(finalPath, existsError) && !existsError)
		{
			CleanupTemporaryFile(temporaryPath);
			outErrorMessage.clear();
			return true;
		}

		CleanupTemporaryFile(temporaryPath);
		outErrorMessage = std::format("Failed to move temporary file '{}' into place as '{}'", temporaryPath.string(), finalPath.string());
		return false;
	}

	bool TryPublishFileSet(std::span<const FilePublication> files, std::string& outErrorMessage)
	{
		FileSetPublisher publisher;
		return publisher.Publish(files, outErrorMessage);
	}

	void CleanupTemporaryFile(const std::filesystem::path& temporaryPath, std::ofstream* output) noexcept
	{
		if (output != nullptr && output->is_open())
		{
			output->close();
		}

		std::error_code errorCode;
		std::filesystem::remove(temporaryPath, errorCode);
	}
}
