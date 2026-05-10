#pragma once

#include "TextureCookRequestList.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

	class TextureAssetCooker;

	struct TextureCookRequestTiming final
	{
		std::uint64_t elapsedMilliseconds = 0;
		TextureAssetId assetId = InvalidTextureAssetId;
		std::filesystem::path sourcePath;
	};

	class TextureCookRequestBatchProcessor final
	{
	  public:
		int CookRequestFile(
			const std::filesystem::path& requestFilePath,
			const std::filesystem::path& summaryPath = {}) const;

	  private:
		class ScopedComInitializer final
		{
		  public:
			ScopedComInitializer() = default;
			~ScopedComInitializer();

			bool TryInitialize(std::string& outErrorMessage);

		  private:
			long m_result = 1;
		};

		static bool TryLoadRequests(
			const std::filesystem::path& requestFilePath,
			std::vector<TextureCookRequest>& outRequests,
			std::string& outErrorMessage);

		bool TryProcessRequest(
			const TextureCookRequest& request,
			TextureAssetCooker& cooker,
			std::size_t& outCookedCount,
			std::string& outErrorMessage) const;

		static void PrintSummary(
			const std::filesystem::path& requestFilePath,
			std::size_t requestCount,
			std::size_t cookedCount,
			std::uint64_t elapsedMilliseconds,
			const std::vector<TextureCookRequestTiming>& requestTimings);

		static bool WriteSummary(
			const std::filesystem::path& summaryPath,
			const std::filesystem::path& requestFilePath,
			std::size_t requestCount,
			std::size_t cookedCount,
			std::uint64_t elapsedMilliseconds,
			const std::vector<TextureCookRequestTiming>& requestTimings,
			std::string& outErrorMessage);

	};