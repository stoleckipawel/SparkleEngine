#pragma once

#include <memory>

class IRayReconstructionProvider;

std::unique_ptr<IRayReconstructionProvider> CreateConfiguredRayReconstructionProvider();
