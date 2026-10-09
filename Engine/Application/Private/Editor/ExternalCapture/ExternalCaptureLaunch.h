#pragma once

#include "Editor/Public/Viewport/ViewportToolbarActions.h"

#include <memory>

std::unique_ptr<ViewportToolbarActions> CreateRequestedCaptureToolbarActions();
