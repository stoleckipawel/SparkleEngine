#pragma once

#include "TextureCookRequestList.h"
#include "TaskTypes.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct TextureCookBatchItemResult final
{
	std::filesystem::path StagedOutputPath;
	TaskResult CookResult = TaskResult::Cancelled("Texture cook did not execute.");
};

class TextureCookBatchExecutor final
{
public:
	TextureCookBatchExecutor() = delete;
	static std::vector<TextureCookBatchItemResult> Execute(const std::vector<TextureCookRequest>& requests, std::size_t memoryBudgetBytes);
};
