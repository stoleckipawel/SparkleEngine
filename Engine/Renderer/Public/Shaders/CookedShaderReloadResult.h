#pragma once

#include <string>
#include <utility>

struct CookedShaderReloadResult final
{
	bool Succeeded = false;
	std::string ErrorMessage;

	static CookedShaderReloadResult Success()
	{
		CookedShaderReloadResult result;
		result.Succeeded = true;
		return result;
	}

	static CookedShaderReloadResult Failure(std::string errorMessage)
	{
		CookedShaderReloadResult result;
		result.ErrorMessage = std::move(errorMessage);
		return result;
	}

	explicit operator bool() const noexcept { return Succeeded; }
};
