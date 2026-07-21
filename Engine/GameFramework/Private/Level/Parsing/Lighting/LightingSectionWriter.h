#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"

#include <fstream>

namespace LevelParsing
{
	void WriteLightingSectionValues(std::ofstream& output, const LevelDesc& levelDesc);
}
