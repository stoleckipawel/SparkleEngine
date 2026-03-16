#include "PCH.h"
#include "Level/Level.h"

Level::Level(LevelDesc levelDesc, std::filesystem::path sourcePath) :
    m_levelDesc(std::move(levelDesc)),
    m_sourcePath(std::move(sourcePath))
{
}