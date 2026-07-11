#pragma once

#include <memory>

class IUpscalerProvider;

std::unique_ptr<IUpscalerProvider> CreateConfiguredUpscalerProvider();
