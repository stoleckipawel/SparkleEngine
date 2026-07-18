#pragma once

#include "TextureCookRequestList.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct TextureCookBatchItemResult final
{
	bool Succeeded = false;
	std::filesystem::path StagedOutputPath;
	std::string Diagnostic;
};

struct TextureCookBatchExecutionResult final
{
	bool Succeeded = false;
	std::size_t PeakAdmittedBytes = 0;
	std::vector<TextureCookBatchItemResult> Items;
};

class TextureCookBatchExecutor final
{
  public:
	TextureCookBatchExecutor() = delete;
	static TextureCookBatchExecutionResult Execute(const std::vector<TextureCookRequest>& requests, std::size_t memoryBudgetBytes);
};
