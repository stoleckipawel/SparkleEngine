# Renderer Viewport And Diagnostics

Status: Renderer feature-family index

Scope: route viewport-facing products, capture and observation, immutable UI composition, and editor/host publication boundaries

| Document | Open it for |
| --- | --- |
| [Diagnostics, Products, And Capture](DiagnosticsProductsAndCapture.md) | diagnostic observations, render products, capture requests/results, provenance, bounds, and failure |
| [UI And Viewport Composition](UiAndViewportComposition.md) | immutable UI packets, viewport texture identity, composition order, host/editor integration, and lifetime |

Diagnostics and capture own observable facts and products. UI/viewport composition owns how already-produced data reaches a viewport; it does not recompute diagnostic truth. The parent [Renderer Feature Dossiers](../README.md) index owns capability routing.
