#pragma once

#include "TextureCookRequestList.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace AssetAuthoring
{
	class TextureAssetCooker;

	class TextureCookRequestBatchProcessor final
	{
	  public:
		int CookRequestFile(const std::filesystem::path& requestFilePath) const;

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
			std::size_t& outSkippedCount,
			std::string& outErrorMessage) const;

		static void PrintSummary(
			const std::filesystem::path& requestFilePath,
			std::size_t requestCount,
			std::size_t cookedCount,
			std::size_t skippedCount);

		static void PrintProcessedRequest(const TextureCookRequest& request);
	};
}