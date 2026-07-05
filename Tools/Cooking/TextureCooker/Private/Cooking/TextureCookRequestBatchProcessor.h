#pragma once

#include "TextureCookRequestList.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

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
			std::string& outErrorMessage) const;

	};
