#pragma once

#include "Tasks/Public/TaskExecution.h"
#include "Textures/CookedTextureLoader.h"

#include <filesystem>
#include <memory>

class TaskExecutor;
class TaskScope;

class CookedTextureLoadTask final
{
public:
	struct Payload final
	{
		CookedTextureFilePayload File;
		LoadedTextureData Texture;
	};

	static TaskExecution Launch(
	    TaskExecutor& taskExecutor,
	    TaskScope& taskScope,
	    const std::filesystem::path& path,
	    const std::shared_ptr<Payload>& payload);
};
