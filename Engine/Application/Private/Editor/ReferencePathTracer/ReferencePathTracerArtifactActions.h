#pragma once

#include <cstdint>

class ReferencePathTracerArtifactCoordinator;
class Renderer;
struct ViewportRenderProducts;
enum class ViewportOutputAction : std::uint8_t;

void ApplyReferencePathTracerArtifactAction(
    ViewportOutputAction action,
    ReferencePathTracerArtifactCoordinator& artifacts,
    Renderer& renderer,
    const ViewportRenderProducts& products);
