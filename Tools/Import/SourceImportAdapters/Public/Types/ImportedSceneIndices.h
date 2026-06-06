#pragma once

#include <cstdint>
#include <limits>

using ImportedMaterialIndex = std::uint32_t;
using ImportedMeshPrimitiveIndex = std::uint32_t;
using ImportedMeshInstanceIndex = std::uint32_t;
using ImportedMeshInstanceGroupIndex = std::uint32_t;

constexpr ImportedMaterialIndex kInvalidImportedMaterialIndex = (std::numeric_limits<ImportedMaterialIndex>::max)();
constexpr ImportedMeshPrimitiveIndex kInvalidImportedMeshPrimitiveIndex = (std::numeric_limits<ImportedMeshPrimitiveIndex>::max)();
constexpr ImportedMeshInstanceIndex kInvalidImportedMeshInstanceIndex = (std::numeric_limits<ImportedMeshInstanceIndex>::max)();
constexpr ImportedMeshInstanceGroupIndex kInvalidImportedMeshInstanceGroupIndex =
    (std::numeric_limits<ImportedMeshInstanceGroupIndex>::max)();
