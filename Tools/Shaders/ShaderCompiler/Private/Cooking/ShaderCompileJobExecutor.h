#pragma once

#include "Cooking/ShaderCompileJob.h"

class ShaderCompileJobExecutor final
{
public:
	ShaderCompileJobExecutor() = delete;

	static ShaderCompileResult Execute(const ShaderCompileJob& job);
};
